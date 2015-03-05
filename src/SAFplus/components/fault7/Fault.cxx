/* Standard headers */
#include <string>
/* SAFplus headers */

#include <clCommon.hxx>
#include <clIocPortList.hxx>
#include <clFaultIpi.hxx>
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
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <clNameApi.hxx>
#include <clIocPortList.hxx>
#include <clHandleApi.hxx>
#include <clFaultPolicyPlugin.hxx>
#include <FaultStatistic.hxx>
#include <FaultHistoryEntity.hxx>
#include <clObjectMessager.hxx>
#include <time.h>

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;
#define FAULT "FLT"
#define FAULT_ENTITY "ENT"
#define FAULT_SERVER "SVR"
namespace SAFplus
{
	FaultSharedMem fsm;
	static unsigned int MAX_MSGS=25;
	static unsigned int MAX_HANDLER_THREADS=10;
    typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;
    FaultPolicyMap faultPolicies;
    // Register a Fault Entity to Fault Server
    void Fault::sendFaultAnnounceMessage(SAFplus::Handle other, SAFplus::FaultState state)
    {
        assert(other != INVALID_HDL);  // We must always report the state of a particular entity, even if that entity is myself (i.e. reporter == other)
    	FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_JOIN;
        sndMessage.state = state;
        sndMessage.faultEntity=other;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_LOCAL_SERVER);
    }
    //deregister Fault Entity
    void Fault::deRegister()
    {
        deRegister(INVALID_HDL);
    }
    //deregister Fault Entity by Entity Handle
    void Fault::deRegister(SAFplus::Handle faultEntity)
    {
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = reporter;    typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;
        FaultPolicyMap faultPolicies;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_LEAVE;
        sndMessage.state = FaultState::STATE_UP;
        sndMessage.faultEntity=faultEntity;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        logDebug(FAULT,FAULT_ENTITY,"Deregister fault entity : Node [%d], Process [%d] , Message Type [%d]", reporter.getNode(), reporter.getProcess(),sndMessage.messageType);
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_LOCAL_SERVER);
    }

    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageSendMode messageMode,SAFplusI::FaultMessageTypeT msgType,SAFplus::FaultPolicy pluginId,SAFplus::FaultEventData faultData)
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
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_LOCAL_SERVER);
    }
    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageSendMode messageMode,SAFplusI::FaultMessageTypeT msgType,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId)
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
    void Fault::notify(SAFplus::Handle faultEntity,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_LOCAL_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,alarmState,category,severity,cause,pluginId);
    }
    void Fault::notifytoActive(SAFplus::Handle faultEntity,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,alarmState,category,severity,cause,pluginId);
    }
    void Fault::notify(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_LOCAL_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,pluginId,faultData);
    }
    void Fault::notifytoActive(SAFplus::Handle faultEntity,SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(faultEntity,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,pluginId,faultData);
    }
    void Fault::notify(SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(reporter,FaultMessageSendMode::SEND_TO_LOCAL_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,pluginId,faultData);
    }
    void Fault::notifytoActive(SAFplus::FaultEventData faultData,SAFplus::FaultPolicy pluginId)
    {
        sendFaultEventMessage(reporter,FaultMessageSendMode::SEND_TO_ACTIVE_SERVER,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,pluginId,faultData);
    }
    //Sending a fault notification to fault server
    void Fault::sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode)
    {
    	logDebug(FAULT,FAULT_ENTITY,"sendFaultNotification");
        switch(messageMode)
        {
            case SAFplusI::FaultMessageSendMode::SEND_TO_LOCAL_SERVER:
            {
                logDebug(FAULT,FAULT_ENTITY,"Send Fault Notification to local Fault server");
                try
                {
                    faultMsgServer->SendMsg(iocFaultLocalServer, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...) // SAFplus::Error &e)
                {
                    logDebug(FAULT,FAULT_ENTITY,"Failed to send.");
                }
                break;
            }
            case SAFplusI::FaultMessageSendMode::SEND_TO_ACTIVE_SERVER:
            {
                // get active server address
            	SAFplus::Handle iocFaultActiveServer=getActiveServerAddress();
                logDebug(FAULT,FAULT_ENTITY,"Send Fault Notification to active Fault Server  : node Id [%d]",iocFaultActiveServer.getNode());
                try
                {
                	faultMsgServer->SendMsg(iocFaultLocalServer, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
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

    void Fault::init(SAFplus::Handle faultHandle,SAFplus::Handle faultServer, int comPort,SAFplus::Wakeable& execSemantics)
    {
        reporter = faultHandle;
        faultCommunicationPort = comPort;
        if(!faultMsgServer)
        {          
            faultMsgServer = &safplusMsgServer;
        }
        iocFaultLocalServer = faultServer;
        registerMyself();
    }

    SAFplus::Handle Fault::getActiveServerAddress()
    {
        SAFplus::Handle masterAddress = fsm.faultHdr->iocFaultServer;
        return masterAddress;
    }
    void Fault::setName(const char* entityName)
    {
        logTrace(FAULT,FAULT_ENTITY,"Set Fault Entity name");
        strncpy(name,entityName,FAULT_NAME_LEN);
    }

    Fault::Fault(SAFplus::Handle faultHandle,const char* name,SAFplus::Handle iocServerAddress)
    {
        wakeable = NULL;
        faultMsgServer = NULL;
        reporter =INVALID_HDL;
        this->init(faultHandle,iocServerAddress,SAFplusI::FLT_IOC_PORT,BLOCK);
        if (name && name[0] != 0)
        {
            setName(name);
        }
    }
    SAFplus::FaultState Fault::getFaultState(SAFplus::Handle faultHandle)
    {
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsm.faultMap->find(faultHandle);
        if (entryPtr == fsm.faultMap->end())
        {
            logError(FAULT,FAULT_ENTITY,"Fault Entity not available in shared memory");
            return SAFplus::FaultState::STATE_UNDEFINED;
        }
        FaultShmEntry *fse = &entryPtr->second;
        logDebug(FAULT,FAULT_ENTITY,"Fault state of Fault Entity [%lx:%lx] is [%s]",faultHandle.id[0],faultHandle.id[1],strFaultEntityState[int(fse->state)]);
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
                for (i=svr->fsmServer.faultMap->begin(); i!=svr->fsmServer.faultMap->end();i++)
                {
                    FaultShmEntry& ge = i->second;
                    Handle handle = i->first;
                    logDebug("GMS","IOC","  Checking fault [%lx:%lx]", handle.id[0],handle.id[1]);
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

    void loadFaultPlugins()
    {
        // pick the SAFplus directory or the current directory if it is not defined.
        const char * soPath = (SAFplus::ASP_APP_BINDIR[0] == 0) ? ".":SAFplus::ASP_APP_BINDIR;
        logDebug(FAULT,"POL","loadFaultPlugins policy: %s", soPath);
        boost::filesystem::path p(soPath);
        boost::filesystem::directory_iterator it(p),eod;
        BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod))
        {
            if (p.extension()==".so")
            {
                if (p.string().find("Fault") != std::string::npos)
                {
                    if(is_regular_file(p))
                    {
                        const char *s = p.c_str();
                        logDebug(FAULT,"POL","Loading policy: %s", s);
                        ClPluginHandle* plug = clLoadPlugin(FAULT_POLICY_PLUGIN_ID,FAULT_POLICY_PLUGIN_VER,s);
                        if (plug)
                        {
                            FaultPolicyPlugin* pp = dynamic_cast<FaultPolicyPlugin*> (plug->pluginApi);
                            if (pp)
                            {
                                faultPolicies[pp->policyId] = plug;
                                logDebug(FAULT,"POL","AMF Policy [%d] plugin load succeeded.", int(pp->pluginId));
                            }
                        }
                        else logError(FAULT,"POL","AMF Policy plugin load failed.  Incorrect plugin type");
                    }
                    else logError(FAULT,"POL","Policy load failed.  Incorrect plugin Identifier or version.");
                }
            }
        }
    }
    void FaultServer::writeToSharedMemoryAllEntity()
    {
        for (Checkpoint::Iterator i=faultCheckpoint.begin();i!=faultCheckpoint.end();i++)
        {
            SAFplus::Checkpoint::KeyValuePair& item = *i;
            Handle tmpHandle = *((Handle*) (*item.first).data);
            FaultShmEntry* tmpShmEntry = ((FaultShmEntry*) (*item.first).data);
            logDebug(FAULT,FAULT_SERVER,"Fault server sync : Fault Entity with Node Id [%d] and Process Id [%d]",tmpHandle.getNode(),tmpHandle.getProcess());
            registerFaultEntity(tmpShmEntry,tmpHandle,false);
        }
    }
    void FaultServer::init()
    {

        faultServerHandle = Handle::create();
        logDebug(FAULT,FAULT_SERVER,"Initial Fault Server Group with Node address [%d]",faultServerHandle.getNode());
        group=Group(FAULT_GROUP);
        group.setNotification(*this);
        SAFplus::objectMessager.insert(faultServerHandle,this);
        faultInfo.iocFaultServer = getProcessHandle(SAFplusI::FLT_IOC_PORT,SAFplus::ASP_NODEADDR);
        group.registerEntity(faultServerHandle, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
        SAFplus::Handle activeMember = INVALID_HDL;
        activeMember = group.getActive();
        logDebug(FAULT,FAULT_SERVER,"Fault Server active address : nodeAddress [%d] - port [%d]",activeMember.getNode(),activeMember.getPort());
        logDebug(FAULT,FAULT_SERVER,"Initialize shared memory");
        fsmServer.init(activeMember);
        fsmServer.clear();  // I am the node representative just starting up, so members may have fallen out while I was gone.  So I must delete everything I knew.
        logDebug(FAULT,FAULT_SERVER,"Loading Fault Policy");
        loadFaultPlugins();
        logDebug(FAULT,FAULT_SERVER,"Register fault message server");
        faultMsgServer = &safplusMsgServer;
        if (1)
        {
            faultMsgServer->RegisterHandler(SAFplusI::FAULT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
        }
        //Fault server include fault client
        if(activeMember.getNode() != SAFplus::ASP_NODEADDR)
        {
            logDebug(FAULT,FAULT_SERVER,"Standby fault server . Get fault information from active fault server");
            faultCheckpoint.name = "safplusFault";
            faultCheckpoint.init(FAULT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED , 1024*1024, CkptDefaultRows);
            logDebug(FAULT,FAULT_SERVER,"Get all checkpoint data");
            sleep(10);// TODO: do not sleep for an arbitrary amount.  Figure out how to check that the conditions are ok to continue.
            writeToSharedMemoryAllEntity();
        }
        else
        {
            logDebug(FAULT,FAULT_SERVER,"Active fault server.");
            faultCheckpoint.name = "safplusFault";
            faultCheckpoint.init(FAULT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED , 1024*1024, CkptDefaultRows);
            sleep(10);
        }
        faultClient = Fault();
        SAFplus::Handle server = faultInfo.iocFaultServer;
        logInfo(FAULT,"CLT","********************Initial fault client*********************");
        faultClient.init(faultServerHandle,server,SAFplusI::FLT_IOC_PORT,BLOCK);
    }

    void FaultServer::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        const SAFplus::FaultMessageProtocol *rxMsg = (SAFplus::FaultMessageProtocol *)msg;
        SAFplusI::FaultMessageTypeT msgType=  rxMsg->messageType;
        SAFplus::Handle reporterHandle = rxMsg->reporter;
        SAFplus::FaultState faultState = rxMsg->state;
        SAFplus::FaultPolicy pluginId = rxMsg->pluginId;
        FaultEventData eventData= rxMsg->data;
        SAFplus::Handle faultEntity=rxMsg->faultEntity;
        Handle fromHandle=rxMsg->reporter;
        //logDebug(FAULT,"MSG","Received message from node [%d] and state [%s]",fromHandle.getNode(),strFaultEntityState[int(faultState)]);
        FaultShmHashMap::iterator entryPtr;
//        if(msgType==SAFplusI::FaultMessageTypeT::MSG_FAULT_SYNC_REQUEST)
//        {
//            sendFaultSyncReply(fromHandle);
//            return;
//        }
//        else if(msgType==SAFplusI::FaultMessageTypeT::MSG_FAULT_SYNC_REPLY)
//        {
//            ClWordT len = msglen - sizeof(FaultMessageProtocol) - 1;
//            fsmServer.applyFaultSync((char *)(rxMsg->syncData),len);
//        }
        if(faultEntity==INVALID_HDL)
        {
            assert(0); // Should never happen now
        }
        else
        {
        	logDebug(FAULT,"MSG","Received fault message with process id [%d]",faultEntity.getProcess());
        	entryPtr = fsmServer.faultMap->find(faultEntity);
        }
        FaultShmEntry *fe=new FaultShmEntry();  // TODO: are these leaked?
        //check point data
        char handleData[sizeof(Buffer)-1+sizeof(Handle)];
        Buffer* key = new(handleData) Buffer(sizeof(Handle));
        Handle* tmp = (Handle*)key->data;
        //logDebug(FAULT,"MSG","Size of Handle [%lu]",sizeof(Handle));
        //logDebug(FAULT,"MSG","Size of shared memory entry [%lu]",sizeof(FaultEntryData));
        char vdata[sizeof(Buffer)-1+sizeof(FaultEntryData)];
        Buffer* val = new(vdata) Buffer(sizeof(FaultEntryData));
        FaultEntryData* tmpShrEntity = (FaultEntryData*)val->data;
        for(int i=0;i<MAX_FAULT_DEPENDENCIES;i++)
        {
            tmpShrEntity->depends[i]=INVALID_HDL;
        }

        if (entryPtr == fsmServer.faultMap->end())  // We don't know about the fault entity, so create it;
        {
            if(msgType!=SAFplusI::FaultMessageTypeT::MSG_ENTITY_JOIN && msgType!=SAFplusI::FaultMessageTypeT::MSG_ENTITY_JOIN_BROADCAST)
            {
                logDebug(FAULT,"MSG","Fault entity not available in shared memory. Ignore this message");
                return;
            }
            logDebug(FAULT,"MSG","Fault entity not available in shared memory. Initial new fault entity");
            if(faultEntity==INVALID_HDL)
            {
                logDebug(FAULT,"MSG","Init reporter handle");
                fe->init(reporterHandle);
                *tmp=reporterHandle;
                tmpShrEntity->faultHdl=reporterHandle;
            }
            else
            {
                logDebug(FAULT,"MSG","Init fault entity handle");
                fe->init(faultEntity);
                *tmp=faultEntity;
                tmpShrEntity->faultHdl=faultEntity;
            }
            tmpShrEntity->dependecyNum=0;
            for(int i=0;i<MAX_FAULT_DEPENDENCIES;i++)
            {
                tmpShrEntity->depends[i]=INVALID_HDL;
            }
        }
        else
        {
            logDebug(FAULT,"MSG","Fault entity available in shared memory. process fault message");
            fe = &entryPtr->second; // &(gsm.groupMap->find(grpHandle)->second);
            tmpShrEntity->dependecyNum= fe->dependecyNum;
            tmpShrEntity->faultHdl= fe->faultHdl;
            tmpShrEntity->state= fe->state;
            for(int i=0;i<MAX_FAULT_DEPENDENCIES;i++)
            {
            	tmpShrEntity->depends[i]=fe->depends[i];
            }
        *tmp=faultEntity;
        }
        if(rxMsg == NULL)
        {
            logError(FAULT,"MSG","Received NULL message. Ignored fault message");
            return;
        }
        switch(msgType)
        {
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_JOIN:
                if(1)
                {
                fe->state=faultState;
                fe->dependecyNum=0;
                tmpShrEntity->state=faultState;
                tmpShrEntity->dependecyNum=0;
                logDebug(FAULT,"MSG","Entity JOIN message with fault state [%d]",fe->state);
                registerFaultEntity(fe,faultEntity,true);
                logDebug(FAULT,"MSG","write to checkpoint");
                faultCheckpoint.write(*key,*val);
                } break;
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_LEAVE:
              if(1)
                {
                logDebug(FAULT,"MSG","Entity Fault message from local node . Deregister fault entity.");
                removeFaultEntity(faultEntity,true);
                //remove entity in checkpoint
                faultCheckpoint.remove(*key);
                } break;
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT:
                if(1)
                {
                    logDebug(FAULT,"MSG","Process fault event message");
                    processFaultEvent(pluginId,eventData,faultEntity,reporterHandle);
                    logDebug("POL","AMF","Fault event data severity [%s] , cause [%s] , catagory [%s] , state [%d] ", SAFplus::strFaultSeverity[int(eventData.severity)],SAFplus::strFaultProbableCause[int(eventData.cause)],SAFplus::strFaultCategory[int(eventData.category)],eventData.alarmState);
                    FaultHistoryEntity faultHistoryEntry;
                    time_t now;
                    time(&now);
                    if (1)
                    {
                        ScopedLock<Mutex> lock(faultServerMutex);
                        faultHistoryEntry.setValue(eventData,faultEntity,reporterHandle,now,NO_TXN);
                        faultHistory.setCurrent(faultHistoryEntry,NO_TXN);
                        faultHistory.setHistory10min(faultHistoryEntry,NO_TXN);
                    }
                    logDebug(FAULT,"MSG","Send fault event message to all fault server");
                    if(reporterHandle.getNode()==SAFplus::ASP_NODEADDR)
                    {
                        FaultMessageProtocol sndMessage;
                        sndMessage.reporter = reporterHandle;
                        sndMessage.messageType = SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT_BROADCAST;
                        sndMessage.data.alarmState= eventData.alarmState;
                        sndMessage.data.category=eventData.category;
                        sndMessage.data.cause=eventData.cause;
                        sndMessage.data.severity=eventData.severity;
                        sndMessage.faultEntity=faultEntity;
                        sndMessage.pluginId=pluginId;
                        sndMessage.syncData[0]=0;
                        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
                        int i;
                        for(i=0;i<faultHistory.history10min.value.size();i++)
                        {
                            char todStr[128]="";
                            struct tm tmStruct;
                            localtime_r(&faultHistory.history10min.value[i].time, &tmStruct);
                            strftime(todStr, 128, "[%b %e %T]", &tmStruct);
                            logDebug(FAULT,"HIS","*** Fault event [%d] : time [%s] node [%d] with processId [%d]",i,todStr,faultHistory.history10min.value[i].faultHdl.getNode(),faultHistory.history10min.value[i].faultHdl.getProcess());
                        }
                    }
                }
                break;
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_JOIN_BROADCAST:
                if(1)
                {
                    if(fromHandle.getNode()==SAFplus::ASP_NODEADDR)
                    {
                        logDebug(FAULT,"MSG","Fault entity join message broadcast from local . Ignore this message");
                    }
                    else
                    {
                        logDebug(FAULT,"MSG","Fault event message broadcast from external.");
                        //TODO Process this event
                        fe->state=faultState;
                        fe->dependecyNum=0;
                        //logDebug(FAULT,"MSG","Register new entity [%lx:%lx] with fault state [%d]",faultEntity.id[0],faultEntity.id[1],fe->state);
                        registerFaultEntity(fe,faultEntity,false);
                    }
                }
             break;
             case SAFplusI::FaultMessageTypeT::MSG_ENTITY_LEAVE_BROADCAST:
                 if(1)
                 {
                     if(fromHandle.getNode()==SAFplus::ASP_NODEADDR)
                     {
                        logDebug(FAULT,"MSG","Fault entity leave message broadcast from local . Ignore this message");
                     }
                     else
                     {
                         logDebug(FAULT,"MSG","Fault event message broadcast from external.");
                         //TODO process this event
                         logDebug(FAULT,"MSG","Entity Fault message from local node . Deregister fault entity.");
                                removeFaultEntity(faultEntity,false);
                     }
                 }
             break;
             case SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT_BROADCAST:
             if(1)
             {
                 if(fromHandle.getNode()==SAFplus::ASP_NODEADDR)
                 {
                     logDebug(FAULT,"MSG","Fault event message broadcast from local . Ignore this message");
                 }
                 else
                 {
                     logDebug(FAULT,"MSG","Fault event message broadcast from external.");
                     //TODO
                 }
             }
             break;
          default:
            logDebug(FAULT,"MSG","Unknown message type [%d] from node [%d]",rxMsg->messageType,fromHandle.getNode());
            break;
        }
    }

//	void FaultServer::sendFaultSyncRequest(SAFplus::Handle activeAddress)
//	{
//		// send sync request to master fault server
//    	logDebug(FAULT,FAULT,"Sending sync requst message to server");
//        FaultMessageProtocol sndMessage;
//        sndMessage.reporter = INVALID_HDL;
//        sndMessage.messageType = FaultMessageTypeT::MSG_FAULT_SYNC_REQUEST;
//        sndMessage.state = FaultState::STATE_UNDEFINED;
//        sndMessage.faultEntity=INVALID_HDL;
//        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
//        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
//        sndMessage.syncData[0]=0;
//        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
//    }
//
//    void FaultServer::sendFaultSyncReply(SAFplus::Handle address)
//    {
//        // send sync reply  to standby fault server
//        long bufferSize=0;
//        char* buf = new char[MAX_FAULT_BUFFER_SIZE];
//        fsmServer.getAllFaultClient(buf,bufferSize);
//        char msgPayload[sizeof(FaultMessageProtocol)-1 + bufferSize];
//        FaultMessageProtocol *sndMessage = (FaultMessageProtocol *)&msgPayload;
//        sndMessage->reporter = INVALID_HDL;
//        sndMessage->messageType = FaultMessageTypeT::MSG_FAULT_SYNC_REPLY;
//        sndMessage->state = FaultState::STATE_UNDEFINED;
//        sndMessage->faultEntity=INVALID_HDL;
//        sndMessage->data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
//        sndMessage->pluginId=SAFplus::FaultPolicy::Undefined;
//        memcpy(sndMessage->syncData,(const void*) &buf,sizeof(GroupIdentity));
//        faultMsgServer->SendMsg(address, (void *)msgPayload, sizeof(msgPayload), SAFplusI::FAULT_MSG_TYPE);
//    }

    //count fault event of fault entity in latest time second
    int FaultServer::countFaultEvent(SAFplus::Handle reporter,SAFplus::Handle faultEntity,long timeInterval)
    {
        int i;
        int count =0;
        for(i = faultHistory.history10min.value.size()-1;i>=0;i--)
        {
            time_t now;
            time(&now);
            if(((now - faultHistory.history10min.value[i].time) <=timeInterval))
            {
                if(faultHistory.history10min.value[i].faultHdl == faultEntity)// && faultHistory.history10min.value[i].reporter != reporter)
                {
                    count++;
                }
            }
            else
            {
            	return count;
            }
        }
        return count;
    }
    //register a fault client entity
    void FaultServer::registerFaultEntity(FaultShmEntry* frp, SAFplus::Handle faultClient,bool needNotify )
    {
    	logDebug(FAULT,"MSG","Register fault entity [%lx:%lx] node id [%d] and process id [%d] initial state [%s]",faultClient.id[0], faultClient.id[1], faultClient.getNode(),faultClient.getProcess(),strFaultEntityState[(int)frp->state]);
        fsmServer.createFault(frp,faultClient);
        if(needNotify)
        {
            logDebug(FAULT,"MSG","Send broadcast fault entity join message :  node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
            broadcastEntityAnnounceMessage(faultClient,frp->state);
        }
    }   
    //Deregister a fault client entity

    void FaultServer::removeFaultEntity(SAFplus::Handle faultClient,bool needNotify)
    {
        //TODO
        logDebug(FAULT,FAULT_SERVER,"De-register fault entity: node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
        fsmServer.faultMap->erase(faultClient);
        if(needNotify)
        {
            logDebug(FAULT,FAULT_SERVER,"Broadcast leave message:  node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
            sendFaultLeaveMessage(faultClient);
        }
    }
    //Remove dependency
    bool FaultServer::removeDependency(SAFplus::Handle dependencyHandle, SAFplus::Handle faultHandle)
    {
        //TODO
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsmServer.faultMap->find(faultHandle);
        if (entryPtr == fsmServer.faultMap->end())
        {
        	return false;
        }
        FaultShmEntry *fse = &entryPtr->second;
        assert(fse);
        if (fse)
        {
            if(fse->dependecyNum==0) return false;
            for(int i=0;i<fse->dependecyNum;i++)
            {
                if(fse->depends[i]==dependencyHandle)
                {
                    for (int j=i;j<fse->dependecyNum-1;j++)
                    {
                        fse->depends[j]=fse->depends[j+1];
                        return false;
                    }
                    fse->dependecyNum--;
                    return true;
                }
            }
            return false;
        }
        else
        {
            //TODO return
            return false;
        }
}
    //Add dependency
    bool FaultServer::setDependency(SAFplus::Handle dependencyHandle,SAFplus::Handle faultHandle)
    {
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsmServer.faultMap->find(faultHandle);
        if (entryPtr == fsmServer.faultMap->end()) return false; // TODO: raise exception
        FaultShmEntry *fse = &entryPtr->second;
        assert(fse);  // If this fails, something very wrong with the group data structure in shared memory.  TODO, should probably delete it and fail the node
        if (fse)
        {
          fse->depends[fse->dependecyNum]=dependencyHandle;
          fse->dependecyNum++;
          return true;
        }
        else
        {
            //TODO return
            return false;
        }
    }   
//    // set name for fault client entity
//    void FaultServer::setName(SAFplus::Handle faultHandle, const char* name)
//    {
//        FaultShmHashMap::iterator entryPtr;
//        entryPtr = fsmServer.faultMap->find(faultHandle);
//        if (entryPtr == fsmServer.faultMap->end()) return; // TODO: raise exception
//        SAFplus::FaultShmEntry *fse = &entryPtr->second;
//        assert(fse);  // If this fails, something very wrong with the group data structure in shared memory.  TODO, should probably delete it and fail the node
//        if (fse) // Name is not meant to change, and not used except for display purposes.  So just change both of them
//        {
//          strncpy(fse->name,name,FAULT_NAME_LEN);
//        }
//        else
//        {
//            //TODO
//        }
//    }
    //process a fault event
    void FaultServer::processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault , SAFplus::Handle faultEntity, SAFplus::Handle faultReporter)
    {
        FaultPolicyMap::iterator it;
        for (it = faultPolicies.begin(); it != faultPolicies.end();it++)
        {
              if(it->first==pluginId || pluginId==FaultPolicy::Undefined)
              {
                  //call Fault plugin to proces fault event
                  FaultPolicyPlugin* pp = dynamic_cast<FaultPolicyPlugin*>(it->second->pluginApi);
                  int count = countFaultEvent(faultReporter,faultEntity,5);
                  FaultAction result = pp->processFaultEvent(fault,faultReporter,faultEntity,count);
                  switch (result)
                  {
                      case FaultAction::ACTION_STOP:
                      {
                          logDebug(FAULT,FAULT_SERVER,"Process fault event return ACTION_STOP.");
                          logDebug(FAULT,FAULT_SERVER,"Try to stop process.");
                          //TODO stop fault process
                          break;
                      }
                      case FaultAction::ACTION_RESTART:
                      {
                          logDebug(FAULT,FAULT_SERVER,"Process fault event return ACTION_RESTART.");
                          logDebug(FAULT,FAULT_SERVER,"Try to restart process.");
                          //TODO stop fault process
                          break;
                      }
                      case FaultAction::ACTION_IGNORE:
                      {
                          logDebug(FAULT,FAULT_SERVER,"Process fault event return ACTION_IGNORE");
                          logDebug(FAULT,FAULT_SERVER,"Ignore this fault event.");
                          break;
                      }
                      default:
                          logDebug(FAULT,"MSG","Un-define return value. Exit");
                          break;
                  }
              }
        }
    }

#if 0
	void FaultServer::processIocNotification(SAFplus::FaultPolicy pluginId,ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId)
	{
        FaultPolicyMap::iterator it;
        for (it = faultPolicies.begin(); it != faultPolicies.end();it++)
          {
              if(it->first==pluginId)
              {
                  //call Fault plugin to proces fault event
                  FaultPolicyPlugin* pp = dynamic_cast<FaultPolicyPlugin*>(it->second->pluginApi);
                  pp->processIocNotification(eventId,nodeAddress,portId);
              }
          }
  	}
#endif
    // broadcast fault state to all other nodes
    void FaultServer::broadcastEntityAnnounceMessage(SAFplus::Handle handle, SAFplus::FaultState state)
    {
        logDebug(FAULT,FAULT_SERVER,"Sending announce message to server with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = faultServerHandle;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_JOIN_BROADCAST;
        sndMessage.state = state;
        sndMessage.faultEntity = handle;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));

    }

    void FaultServer::sendFaultLeaveMessage(SAFplus::Handle handle)
    {
        logDebug(FAULT,FAULT_SERVER,"Sending announce message to server with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = handle;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_LEAVE_BROADCAST;
        sndMessage.state = FaultState::STATE_UP; // TODO: wouldn't state be down or undefined?
        sndMessage.faultEntity=handle;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
    }

    void FaultServer::sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode)
    {
        switch(messageMode)
        {
            case SAFplusI::FaultMessageSendMode::SEND_BROADCAST:
            {
                /* Destination is broadcast address */
                Handle dest = getProcessHandle(faultCommunicationPort,Handle::AllNodes);
                try
                {
                    faultMsgServer->SendMsg(dest, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...)
                {
                    //logDebug("GMS","MSG","Failed to send. Error %x",e.rc);
                    logDebug("GMS","MSG","Failed to send.");
                }
                break;
            }
            default:
            {
                logError("GMS","MSG","Unknown message sending mode");
                break;
            }
        }
    }
    void FaultServer::sendFaultNotificationToGroup(void* data, int dataLength)
    {
        group.send(data,dataLength,GroupMessageSendMode::SEND_BROADCAST);
    }
    FaultServer::FaultServer()
    {

    }
    SAFplus::FaultState FaultServer::getFaultState(SAFplus::Handle faultHandle)
    {
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsmServer.faultMap->find(faultHandle);
        if (entryPtr == fsmServer.faultMap->end())
        {
            logError(FAULT,FAULT,"not available in shared memory");
            return SAFplus::FaultState::STATE_UNDEFINED;
        }
        FaultShmEntry *fse = &entryPtr->second;
    	logError(FAULT,FAULT_SERVER,"Fault Entity [%lx.%lx] State  [%d]",faultHandle.id[0],faultHandle.id[1],fse->state);
        return fse->state;
    }
    void faultInitialize(void)
      {
    	SAFplus::fsm.init(INVALID_HDL);
      }

    void FaultServer::wake(int amt,void* cookie)
    {
         changeCount++;
         Group* g = (Group*) cookie;
         logInfo(FAULT,FAULT_SERVER, "Group [%lx:%lx] changed", g->handle.id[0],g->handle.id[1]);
         Group::Iterator i;
         char buf[100];
         for (i=g->begin(); i != g->end(); i++)
         {
             const GroupIdentity& gid = i->second;
             logInfo(FAULT,FAULT_SERVER, " Entity [%lx:%lx] on node [%d] credentials [%ld] capabilities [%d] %s", gid.id.id[0],gid.id.id[1],gid.id.getNode(),gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
         }
   }
};
