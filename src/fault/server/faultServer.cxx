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
#define FAULT_SERVER "SVR"

extern void setNodeOperState(const SAFplus::Handle& nodeHdl, bool state);

namespace SAFplus
{

    typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;
    FaultPolicyMap faultPolicies;

    void FaultServer::loadFaultPlugins()
    {
        // pick the SAFplus directory or the current directory if it is not defined.
        const char * soPath = ".";
        if (boost::filesystem::is_directory("../plugin")) soPath = "../plugin";
        else if ((SAFplus::ASP_APP_BINDIR[0]!=0) && boost::filesystem::is_directory(SAFplus::ASP_APP_BINDIR))
          {
            soPath = SAFplus::ASP_APP_BINDIR;
          }
        else if (boost::filesystem::is_directory("../lib")) soPath = "../lib";

        logDebug(FAULT,"POL","loadFaultPlugins policy: %s", soPath);
        boost::filesystem::path pth(soPath);

        boost::filesystem::directory_iterator it(pth),eod;
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
                            FaultPolicyPlugin_1* pp = dynamic_cast<FaultPolicyPlugin_1*> (plug->pluginApi);
                            if (pp)
                            {
                                pp->initialize(this);
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
            FaultShmEntry* tmpShmEntry = ((FaultShmEntry*) (*item.second).data);
            logDebug(FAULT,FAULT_SERVER,"Fault server sync : Fault Entity with Node Id [%d] and Process Id [%d]",tmpHandle.getNode(),tmpHandle.getProcess());
            registerFaultEntity(tmpShmEntry,tmpHandle,false);
        }
    }

    void FaultServer::init(const SAFplusI::GroupServer* gs)
    {
        this->gs = const_cast<SAFplusI::GroupServer*>(gs);
        faultServerHandle = Handle::create();  // This is the handle for this specific fault server

        logDebug(FAULT,FAULT_SERVER,"Initialize shared memory");
        fsmServer.init();
        fsmServer.clear();  // I am the node representative just starting up, so members may have fallen out while I was gone.  So I must delete everything I knew.

        logDebug(FAULT,FAULT_SERVER,"Initial Fault Server Group with Node address [%d]",faultServerHandle.getNode());
        group.init(FAULT_GROUP);
        group.setNotification(*this);
        SAFplus::objectMessager.insert(faultServerHandle,this);

        faultMsgServer = &safplusMsgServer;

        // the faultMsgServer.handle is going to be a process handle. faultServerHandle also points to this process (its essentially a superset of the faultMsgServer.handle which == getProcessHandle(SAFplusI::FAULT_IOC_PORT,SAFplus::ASP_NODEADDR) btw).  So let's use that one.
        faultInfo.iocFaultServer = faultServerHandle;
        group.registerEntity(faultServerHandle, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
        SAFplus::Handle activeMember = group.getActive();
        assert(activeMember != INVALID_HDL);  // It can't be invalid because I am available to be active.
        logDebug(FAULT,FAULT_SERVER,"Fault Server active address : nodeAddress [%d] - port [%d]",activeMember.getNode(),activeMember.getPort());
        fsmServer.setActive(activeMember);
        logDebug(FAULT,FAULT_SERVER,"Loading Fault Policy");
        loadFaultPlugins();
        logDebug(FAULT,FAULT_SERVER,"Register fault message server");
        if (1)
        {
            faultMsgServer->RegisterHandler(SAFplusI::FAULT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
        }

        faultCheckpoint.name = "safplusFault";
        faultCheckpoint.init(FAULT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);

        // TODO: I do not think you want to do this on the standby only.  If you are the Active fault server, you still want to populate your data
        // from the checkpoint.  Either the checkpoint will be empty in the case of an initial startup, or it will have good data
        // in the case where there was a dual failure in the fault manager.
        if(activeMember.getNode() != SAFplus::ASP_NODEADDR)
        {
            logDebug(FAULT,FAULT_SERVER,"Standby fault server . Get fault information from active fault server");
            sleep(5);// TODO: do not sleep for an arbitrary amount.  Figure out how to check that the conditions are ok to continue.
            logDebug(FAULT,FAULT_SERVER,"Get all checkpoint data");
            writeToSharedMemoryAllEntity();
        }
        else
        {
            logDebug(FAULT,FAULT_SERVER,"Active fault server.");
            // sleep(5); // TODO: do not sleep for an arbitrary amount.  Figure out how to check that the conditions are ok to continue.
        }
        //faultClient = Fault();
        //SAFplus::Handle server = faultInfo.iocFaultServer;
        //logInfo(FAULT,"CLT","********************Initial fault client*********************");
        //faultClient.init(faultServerHandle,server,SAFplusI::FAULT_IOC_PORT,BLOCK);
    }

    void FaultServer::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        if(msg == NULL)
        {
            logError(FAULT,"MSG","Received NULL message. Ignored fault message");
            return;
        }
        const SAFplus::FaultMessageProtocol *rxMsg = (SAFplus::FaultMessageProtocol *)msg;
        SAFplus::FaultMessageType msgType=  rxMsg->messageType;
        SAFplus::Handle reporterHandle = rxMsg->reporter;
        SAFplus::FaultState faultState = rxMsg->state;
        SAFplus::FaultPolicy pluginId = rxMsg->pluginId;
        FaultEventData eventData= rxMsg->data;
        SAFplus::Handle faultEntity=rxMsg->faultEntity;
        assert(faultEntity!=INVALID_HDL);  // TODO: changed the code so this should never happen.  Later, change this to ignore the message (or mark against the originator because no corrupt message should cause the server to die.
        Handle fromHandle=rxMsg->reporter;
        //logDebug(FAULT,"MSG","Received message from node [%d] port [%d] and about [%lx.%lx] claiming state [%s]",fromHandle.getNode(),fromHandle.getPort(),faultEntity.id[0],faultEntity.id[1],strFaultEntityState[int(faultState)]);
        FaultShmHashMap::iterator entryPtr;

        entryPtr = fsmServer.faultMap->find(faultEntity);

        FaultShmEntry feMem;
        FaultShmEntry *fe=&feMem;
        //check point data
        char handleData[sizeof(Buffer)-1+sizeof(Handle)];
        Buffer* key = new(handleData) Buffer(sizeof(Handle));
        Handle* keyData = (Handle*)key->data;
        char vdata[sizeof(Buffer)-1+sizeof(FaultEntryData)];
        Buffer* val = new(vdata) Buffer(sizeof(FaultEntryData));
        FaultEntryData* tmpShrEntity = (FaultEntryData*)val->data;
        *((Handle*)key->data)=faultEntity;
        bool created = false;

        if (entryPtr == fsmServer.faultMap->end())  // We don't know about the fault entity, so create it;
        {
#if 0
            if(msgType!=SAFplus::FaultMessageType::MSG_ENTITY_JOIN && msgType!=SAFplus::FaultMessageType::MSG_ENTITY_JOIN_BROADCAST)
            {
                logWarning(FAULT,"MSG","Fault report from [%" PRIx64 ":%" PRIx64 "] about an entity [%" PRIx64 ":%" PRIx64 "] not available in shared memory. Ignoring this message",reporterHandle.id[0],reporterHandle.id[1],faultEntity.id[0],faultEntity.id[1]);
                return;
            }
#endif
            logDebug(FAULT,"MSG","Fault entity not available in shared memory. Initialize new entity");
            fe->init(faultEntity);
            tmpShrEntity->faultHdl=faultEntity;
            tmpShrEntity->dependecyNum=0;
            for(int i=0;i<SAFplusI::MAX_FAULT_DEPENDENCIES;i++)
            {
                tmpShrEntity->depends[i]=INVALID_HDL;
            }
            created = true;
        }
        else
        {
            logDebug(FAULT,"MSG","Fault entity available in shared memory. process fault message");
            fe = &entryPtr->second;
            tmpShrEntity->dependecyNum= fe->dependecyNum;
            tmpShrEntity->faultHdl= fe->faultHdl;
            tmpShrEntity->state= fe->state;
            for(int i=0;i<SAFplusI::MAX_FAULT_DEPENDENCIES;i++)
            {
            	tmpShrEntity->depends[i]=fe->depends[i];
            }
        }

        switch(msgType)
        {
            case SAFplus::FaultMessageType::MSG_ENTITY_JOIN:
                if(1)
                {
                  if (created) // This entity is unknown
                    {
                    fe->state=faultState;
                    fe->dependecyNum=0;
                    tmpShrEntity->state=faultState;
                    tmpShrEntity->dependecyNum=0;
                    logDebug(FAULT,"MSG","Entity JOIN message with fault state [%d]",(int)fe->state);
                    registerFaultEntity(fe,faultEntity,true);
                    logDebug(FAULT,"MSG","write to checkpoint");
                    faultCheckpoint.write(*key,*val);
                    }
                  else
                    {
                      logDebug(FAULT,"MSG","Repeated entity JOIN message for entity [%d.%d.%" PRIx64 "], entity handle [%" PRIx64 ":%" PRIx64 "] state %d",faultEntity.getNode(),faultEntity.getProcess(),faultEntity.getIndex(),faultEntity.id[0], faultEntity.id[1], (int)faultState);
                      setFaultState(faultEntity,faultState);
#if 0
                      if (faultEntity.getProcess() == 0 && faultEntity.getNode() != SAFplus::ASP_NODEADDR && faultState == FaultState::STATE_UP) // other node is up
                      {
                        try
                        {
                            Handle& amfMasterHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
                            if (SAFplus::ASP_NODEADDR == amfMasterHdl.getNode()) // only active node needs to do this
                            {
                                setNodeOperState(faultEntity, true);
                            }
                        }
                        catch(NameException& ex)
                        {
                            logError(FAULT,"MSG","getHandle got exception [%s]", ex.what());
                        }
                      }
#endif
                    }
                }
                break;
            case SAFplus::FaultMessageType::MSG_ENTITY_LEAVE:
                if(1)
                {
                    logDebug(FAULT,"MSG","Entity Fault message from local node . Deregister fault entity.");
                    removeFaultEntity(faultEntity,true);
                    //remove entity in checkpoint
                    faultCheckpoint.remove(*key);
                }
                break;
            case SAFplus::FaultMessageType::MSG_ENTITY_FAULT:
                if(1)
                {
                  //logDebug(FAULT,"MSG","Process fault event message");
                    if (created) registerFaultEntity(fe,faultEntity,true);
                    processFaultEvent(pluginId,eventData,faultEntity,reporterHandle);
                    logDebug("POL","AMF","Fault event data severity [%s] , cause [%s] , catagory [%s] , state [%d] ", SAFplus::strFaultSeverity[int(eventData.severity)],SAFplus::strFaultProbableCause[int(eventData.cause)],SAFplus::strFaultCategory[int(eventData.category)],(int)eventData.alarmState);
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
//                    logDebug(FAULT,"MSG","Send fault event message to all fault servers");
//                    if(reporterHandle.getNode()==SAFplus::ASP_NODEADDR)
//                    {
//                        FaultMessageProtocol sndMessage;
//                        sndMessage.reporter = reporterHandle;
//                        sndMessage.messageType = SAFplus::FaultMessageType::MSG_ENTITY_FAULT_BROADCAST;
//                        sndMessage.data.alarmState= eventData.alarmState;
//                        sndMessage.data.category=eventData.category;
//                        sndMessage.data.cause=eventData.cause;
//                        sndMessage.data.severity=eventData.severity;
//                        sndMessage.faultEntity=faultEntity;
//                        sndMessage.pluginId=pluginId;
//                        sndMessage.syncData[0]=0;
//                        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
//                        int i;
//                        for(i=0;i<faultHistory.history10min.value.size();i++)
//                        {
//                            char todStr[128]="";
//                            struct tm tmStruct;
//                            localtime_r(&faultHistory.history10min.value[i].time, &tmStruct);
//                            strftime(todStr, 128, "[%b %e %T]", &tmStruct);
//                            logDebug(FAULT,"HIS","*** Fault event [%d] : time [%s] node [%d] with processId [%d]",i,todStr,faultHistory.history10min.value[i].faultHdl.getNode(),faultHistory.history10min.value[i].faultHdl.getProcess());
//                        }
//                    }
                }
                break;
            case SAFplus::FaultMessageType::MSG_ENTITY_STATE_CHANGE:
                if(1)
                {
                    fe->state=faultState;
                    tmpShrEntity->state=faultState;
                    setFaultState(faultEntity,faultState);
                }
            break;
            case SAFplus::FaultMessageType::MSG_ENTITY_JOIN_BROADCAST:
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
                        //logDebug(FAULT,"MSG","Register new entity [%" PRIx64 ":%" PRIx64 "] with fault state [%d]",faultEntity.id[0],faultEntity.id[1],fe->state);
                        registerFaultEntity(fe,faultEntity,false);
                    }
                }
             break;
             case SAFplus::FaultMessageType::MSG_ENTITY_LEAVE_BROADCAST:
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
             case SAFplus::FaultMessageType::MSG_ENTITY_FAULT_BROADCAST:
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
             case SAFplus::FaultMessageType::MSG_ENTITY_STATE_CHANGE_BROADCAST:
                 if(1)
                 {
                     if(fromHandle.getNode()==SAFplus::ASP_NODEADDR)
                     {
                         logDebug(FAULT,"MSG","Fault entity state change message broadcast from local . Ignore this message");
                     }
                     else
                     {
                         logDebug(FAULT,"MSG","Fault entity state change message broadcast from external.");
                         fsmServer.updateFaultHandleState(faultEntity, faultState);
                     }
                 }
             break;
          default:
            logDebug(FAULT,"MSG","Unknown message type [%d] from node [%d]",(int)rxMsg->messageType,fromHandle.getNode());
            break;
        }
    }

#if 0
    void FaultServer::sendFaultSyncRequest(SAFplus::Handle activeAddress)
    {
        // send sync request to master fault server
        logDebug(FAULT,FAULT,"Sending sync requst message to server");
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = INVALID_HDL;
        sndMessage.messageType = FaultMessageType::MSG_FAULT_SYNC_REQUEST;
        sndMessage.state = FaultState::STATE_UNDEFINED;
        sndMessage.faultEntity=INVALID_HDL;
        sndMessage.data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,SAFplus::AlarmCategory::ALARM_CATEGORY_INVALID,SAFplus::AlarmSeverity::ALARM_SEVERITY_INVALID,SAFplus::AlarmProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
    }

    void FaultServer::sendFaultSyncReply(SAFplus::Handle address)
    {
        // send sync reply  to standby fault server
        long bufferSize=0;
        char* buf = new char[MAX_FAULT_BUFFER_SIZE];
        fsmServer.getAllFaultClient(buf,bufferSize);
        char msgPayload[sizeof(FaultMessageProtocol)-1 + bufferSize];
        FaultMessageProtocol *sndMessage = (FaultMessageProtocol *)&msgPayload;
        sndMessage->reporter = INVALID_HDL;
        sndMessage->messageType = FaultMessageType::MSG_FAULT_SYNC_REPLY;
        sndMessage->state = FaultState::STATE_UNDEFINED;
        sndMessage->faultEntity=INVALID_HDL;
        sndMessage->data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,SAFplus::AlarmCategory::ALARM_CATEGORY_INVALID,SAFplus::AlarmSeverity::ALARM_SEVERITY_INVALID,SAFplus::AlarmProbableCause::ALARM_ID_INVALID);
        sndMessage->pluginId=SAFplus::FaultPolicy::Undefined;
        memcpy(sndMessage->syncData,(const void*) &buf,sizeof(GroupIdentity));
        faultMsgServer->SendMsg(address, (void *)msgPayload, sizeof(msgPayload), SAFplusI::FAULT_MSG_TYPE);
    }
#endif

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
    	logDebug(FAULT,"MSG","Register fault entity [%" PRIx64 ":%" PRIx64 "] node id [%d] and process id [%d] initial state [%s]",faultClient.id[0], faultClient.id[1], faultClient.getNode(),faultClient.getProcess(),strFaultEntityState[(int)frp->state]);
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
    //process a fault event
    void FaultServer::processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault , SAFplus::Handle faultEntity, SAFplus::Handle faultReporter)
    {
        FaultPolicyMap::iterator it;
        for (it = faultPolicies.begin(); it != faultPolicies.end();it++)
        {
              if(it->first==pluginId || pluginId==FaultPolicy::Undefined)
              {
                  //call Fault plugin to proces fault event
                  FaultPolicyPlugin_1* pp = dynamic_cast<FaultPolicyPlugin_1*>(it->second->pluginApi);
                  int count = countFaultEvent(faultReporter,faultEntity,5);
                  bool result = pp->processFaultEvent(fault,faultReporter,faultEntity,count);
                  if (result) break;
              }
        }
        logDebug("FLT","---","have gs to update related entity fault state");
        gs->removeEntities(faultEntity);
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
    // broadcast fault entity join to all other nodes
    void FaultServer::broadcastEntityAnnounceMessage(SAFplus::Handle handle, SAFplus::FaultState state)
    {
        logDebug(FAULT,FAULT_SERVER,"Sending announce message to server with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = faultServerHandle;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_JOIN_BROADCAST;
        sndMessage.state = state;
        sndMessage.faultEntity = handle;
        sndMessage.data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,SAFplus::AlarmCategory::ALARM_CATEGORY_INVALID,SAFplus::AlarmSeverity::ALARM_SEVERITY_INVALID,SAFplus::AlarmProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));

    }

    void FaultServer::broadcastEntityStateChangeMessage(SAFplus::Handle handle, SAFplus::FaultState state)
    {
        logDebug(FAULT,FAULT_SERVER,"Sending announce message to server with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = faultServerHandle;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_STATE_CHANGE_BROADCAST;
        sndMessage.state = state;
        sndMessage.faultEntity = handle;
        sndMessage.data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,SAFplus::AlarmCategory::ALARM_CATEGORY_INVALID,SAFplus::AlarmSeverity::ALARM_SEVERITY_INVALID,SAFplus::AlarmProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));

    }

    void FaultServer::sendFaultLeaveMessage(SAFplus::Handle handle)
    {
        logDebug(FAULT,FAULT_SERVER,"Sending announce message to server with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.reporter = handle;
        sndMessage.messageType = FaultMessageType::MSG_ENTITY_LEAVE_BROADCAST;
        sndMessage.state = FaultState::STATE_UP; // TODO: wouldn't state be down or undefined?
        sndMessage.faultEntity=handle;
        sndMessage.data.init(SAFplus::AlarmState::ALARM_STATE_INVALID,SAFplus::AlarmCategory::ALARM_CATEGORY_INVALID,SAFplus::AlarmSeverity::ALARM_SEVERITY_INVALID,SAFplus::AlarmProbableCause::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sndMessage.syncData[0]=0;
        sendFaultNotificationToGroup((void *)&sndMessage,sizeof(FaultMessageProtocol));
    }

    void FaultServer::sendFaultNotification(void* data, int dataLength, SAFplus::FaultMessageSendMode messageMode)
    {
        switch(messageMode)
        {
            case SAFplus::FaultMessageSendMode::SEND_BROADCAST:
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
        logError(FAULT,FAULT_SERVER,"Fault Entity [%" PRIx64 ":%" PRIx64 "] State  [%d]",faultHandle.id[0],faultHandle.id[1],(int)fse->state);
        return fse->state;
    }


    void FaultServer::wake(int amt,void* cookie)
    {
         changeCount++;
         Group* g = (Group*) cookie;
         logInfo(FAULT,FAULT_SERVER, "Group [%" PRIx64 ":%" PRIx64 "] changed", g->handle.id[0],g->handle.id[1]);
         SAFplus::Handle activeMember = g->getActive();
         fsmServer.setActive(activeMember);
    }

    void FaultServer::setFaultState(SAFplus::Handle handle,SAFplus::FaultState state)
    {
      if (fsmServer.updateFaultHandleState(handle, state))  // If there was a change
        {
        // If the state is DOWN or UP, then mark all children of nodes or processes as also DOWN or UP
        if (state == FaultState::STATE_DOWN || state == FaultState::STATE_UP)
          {
            fsmServer.setChildFaults(handle, state);
          }

        // broadcast this state update to all other fault node representatives
        broadcastEntityStateChangeMessage(handle,state);
        // update fault checkpoint
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsmServer.faultMap->find(handle);
        FaultShmEntry feMem;
        FaultShmEntry *fe=&feMem;
        fe = &entryPtr->second;
        char handleData[sizeof(Buffer)-1+sizeof(Handle)];
        Buffer* key = new(handleData) Buffer(sizeof(Handle));
        Handle* keyData = (Handle*)key->data;
        char vdata[sizeof(Buffer)-1+sizeof(FaultEntryData)];
        Buffer* val = new(vdata) Buffer(sizeof(FaultEntryData));
        FaultEntryData* tmpShrEntity = (FaultEntryData*)val->data;
        *((Handle*)key->data)=handle;
        tmpShrEntity->dependecyNum= fe->dependecyNum;
        tmpShrEntity->faultHdl= fe->faultHdl;
        tmpShrEntity->state= fe->state;
        for(int i=0;i<SAFplusI::MAX_FAULT_DEPENDENCIES;i++)
        {
        	tmpShrEntity->depends[i]=fe->depends[i];
        }
        faultCheckpoint.write(*key,*val);
        }
    }
    void FaultServer::RemoveAllEntity()
    {
        fsmServer.removeAll();
    }


};
