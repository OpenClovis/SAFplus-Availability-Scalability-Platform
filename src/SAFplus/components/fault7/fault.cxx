/* Standard headers */
#include <string>
/* SAFplus headers */

#include <clCommon.hxx>
//#include <clNameApi.hxx>
#include <clIocPortList.hxx>
#include <clFaultIpi.hxx>
#include <clHandleApi.hxx>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clIocPortList.hxx>
#include <clHandleApi.hxx>
#include <clFaultPolicyPlugin.hxx>


using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

namespace SAFplus
{

	FaultMessageProtocol::FaultMessageProtocol()
	{

	}
    // register a fault entity to fault server
    void Fault::sendFaultAnnounceMessage()
    {
    	logDebug("FLT","FLT","sendFaultAnnounceMessage with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.fault = handle;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_JOIN;
        sndMessage.force = 0;
        sndMessage.faultEntity=INVALID_HDL;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_SERVER);
    }

    void Fault::sendFaultEventMessage(SAFplus::Handle faultEntity,SAFplusI::FaultMessageTypeT msgType,SAFplusI::AlarmStateT alarmState,SAFplusI::AlarmCategoryTypeT category,SAFplusI::AlarmSeverityTypeT severity,SAFplusI::AlarmProbableCauseT cause,SAFplus::FaultPolicy pluginId)
    {
    	logDebug("FLT","FLT","sendFaultEventMessage with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.fault = handle;
        sndMessage.messageType = msgType;
        sndMessage.force = 0;
        sndMessage.data.alarmState= alarmState;
        sndMessage.data.category=category;
        sndMessage.data.cause=cause;
        sndMessage.data.severity=severity;
        sndMessage.faultEntity=faultEntity;
        sndMessage.pluginId=pluginId;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),FaultMessageSendMode::SEND_TO_SERVER);
    }

    //send a fault notification to fault server
    void Fault::sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode)
    {
    	logDebug("FLT","MSG","sendFaultNotification");
    	//Todo : remove messageMode ??
        switch(messageMode)
        {
            case SAFplusI::FaultMessageSendMode::SEND_TO_SERVER:
            {
                /* Destination is broadcast address */
            	logDebug("FLT","MSG","sendFaultNotification : send to fault server");
                //logInfo("GMS","MSG","Sending broadcast message");
                try
                {
                    faultMsgServer->SendMsg(iocFaultServer, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
                }
                catch (...) // SAFplus::Error &e)
                {
                    //logDebug("GMS","MSG","Failed to send. Error %x",e.rc);
                    logDebug("FLT","MSG","Failed to send.");
                }
                break;
            }
            default:
            {
                logError("FLT","MSG","Unknown message sending mode");
                break;
            }
        }
    }

    //init a fault entity with handle and comport information
    void Fault::init(SAFplus::Handle faultHandle,ClIocAddressT faultServer, int comPort,SAFplus::Wakeable& execSemantics)
    {
    	logDebug("FLT","FLT","Initial Fault Entity");
        handle = faultHandle;
        faultCommunicationPort = comPort;
        if(!faultMsgServer)
        {
            faultMsgServer = &safplusMsgServer;
        }
        iocFaultServer.iocPhyAddress.nodeAddress = faultServer.iocPhyAddress.nodeAddress;
        iocFaultServer.iocPhyAddress.portId      = faultServer.iocPhyAddress.portId;
        FaultShmHashMap::iterator entryPtr;
        do 
        {
            entryPtr = fsm.faultMap->find(handle);
        	logDebug("FLT","MSG","check Fault Entity in shared memory");
            if (entryPtr == fsm.faultMap->end())
            {
                sendFaultAnnounceMessage();  // This will be sent to fault server 
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));  // TODO use thread change condition
            }
        }while ((&execSemantics == &BLOCK)&&(entryPtr == fsm.faultMap->end()));
    }


    void Fault::setName(const char* entityName)
    {
    	logDebug("FLT","FLT","set fault entity name");
        strncpy(name,entityName,FAULT_NAME_LEN);
    }

    Fault::Fault(SAFplus::Handle faultHandle,const char* name, int comPort,ClIocAddressT iocServerAddress)
    {
    	wakeable = NULL;
        faultMsgServer = NULL;
        handle =INVALID_HDL;
        this->init(faultHandle,iocServerAddress,SAFplusI::GMS_IOC_PORT,BLOCK);
        if (name && name[0] != 0)
        {
            setName(name);
        }
    }
    int Fault::getFaultState(SAFplus::Handle faultHandle)
    {
    	logDebug("FLT","FLT","get fault state");
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsm.faultMap->find(handle);
        if (entryPtr == fsm.faultMap->end())
        {
        	logError("FLT","FLT","not available in shared memory");
            return 0;
        }
        FaultShmEntry *fse = &entryPtr->second;
        return fse->state;
    }
    Fault::Fault()
    {
    	logDebug("FLT","FLT","Fault()");
    	handle=INVALID_HDL;
    	faultMsgServer=NULL;
    }

    ClRcT faultIocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie)
    {
      FaultServer* svr = (FaultServer*) cookie;
      ClRcT rc = CL_OK;
      ClIocAddressT address;
      ClIocNotificationIdT eventId = (ClIocNotificationIdT) ntohl(notification->id);
      ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
      ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
      logDebug("FLT","IOC","Recv notification [%d] for [%d.%d]", eventId, nodeAddress,portId);
      switch(eventId)
      {
      	  case CL_IOC_COMP_DEATH_NOTIFICATION:
          {
        	  logDebug("FLT","IOC","Received component leave notification for [%d.%d]", nodeAddress,portId);
        	  ScopedLock<ProcSem> lock(svr->fsm.mutex);
        	  FaultShmHashMap::iterator i;
        	  for (i=svr->fsm.faultMap->begin(); i!=svr->fsm.faultMap->end();i++)
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

        	ScopedLock<ProcSem> lock(svr->fsm.mutex);
        	FaultShmHashMap::iterator i;

        	for (i=svr->fsm.faultMap->begin(); i!=svr->fsm.faultMap->end();i++)
        	{
        		FaultShmEntry& ge = i->second;
        		//deregister all fault entity in this node.
        	}
        } break;
        default:
        {
        	logInfo("GMS","IOC","Received event [%d] from IOC notification",eventId);
        	break;
        }
        }
      return rc;
    }




    typedef boost::unordered_map<SAFplus::FaultPolicy,ClPluginHandle*> FaultPolicyMap;
    FaultPolicyMap faultPolicies;



    void loadFaultPlugins()
    {
        // pick the SAFplus directory or the current directory if it is not defined.
    	const char * soPath = (SAFplus::ASP_APP_BINDIR[0] == 0) ? ".":SAFplus::ASP_APP_BINDIR;
    	clLogInfo("POL","LOAD","loadFaultPlugins policy: %s", soPath);
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
                        clLogInfo("POL","LOAD","Loading policy: %s", s);
                        ClPluginHandle* plug = clLoadPlugin(FAULT_POLICY_PLUGIN_ID,FAULT_POLICY_PLUGIN_VER,s);
                        if (plug)
                        {
                            FaultPolicyPlugin* pp = dynamic_cast<FaultPolicyPlugin*> (plug->pluginApi);
                            if (pp)
                            {
                                faultPolicies[pp->policyId] = plug;
                                clLogInfo("POL","LOAD","AMF Policy [%d] plugin load succeeded.", int(pp->pluginId));
                            }
                        }
                        else clLogError("POL","LOAD","AMF Policy plugin load failed.  Incorrect plugin type");
                    }
                    else clLogError("POL","LOAD","Policy load failed.  Incorrect plugin Identifier or version.");
                }
            }
        }
    }
    void FaultServer::init()
    {
        logInfo("FLT","INI","initialize shared memory");
        fsm.init();
        logInfo("FLT","INI","initialize shared memory successful");
        fsm.clear();  // I am the node representative just starting up, so members may have fallen out while I was gone.  So I must delete everything I knew.
        logInfo("FLT","INI","register IOC notification");
        clIocNotificationRegister(faultIocNotificationCallback,this);
        if(!faultMsgServer)
        {
            faultMsgServer = &safplusMsgServer;
        }
        loadFaultPlugins();
        if (1)
        {
          faultMsgServer->RegisterHandler(SAFplusI::FAULT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
        }
    }

    void FaultServer::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        logInfo("SYNC","FLT","Received fault message from %d", from.iocPhyAddress.nodeAddress);
    
    /* Parse the message and process if it is valid */
        SAFplus::FaultMessageProtocol *rxMsg = (SAFplus::FaultMessageProtocol *)msg;

        SAFplus::Handle faultHandle = rxMsg->fault;
        FaultShmHashMap::iterator entryPtr = fsm.faultMap->find(faultHandle);
        FaultShmEntry* fe = NULL;
        if (entryPtr == fsm.faultMap->end())  // We don't know about the fault entity, so create it;
        {
            fe->dependecyNum=0;
            strncpy(fe->name,rxMsg->name,FAULT_NAME_LEN);
            fe->state=rxMsg->state;
        }
        else
        {
            fe = &entryPtr->second; // &(gsm.groupMap->find(grpHandle)->second);
        }

        if(rxMsg == NULL)
        {
          logError("FLT","MSG","Received NULL message. Ignored");
          return;
        }
        FaultEventData eventData= rxMsg->data;
        SAFplus::Handle faultEntity=rxMsg->faultEntity;
        logInfo("FLT","MSG","Received message [%x] from node %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
    
        switch(rxMsg->messageType)
        {
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_JOIN:
                if(1)
                {
                    logDebug("FLT","MSG","Entity JOIN message");
                    RegisterFaultEntity(fe,faultHandle,true);
                }break;
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_LEAVE:
            	if(1)
            	{
            		if(from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
            		{
            			logDebug("FLT","MSG","Entity Fault message from local. Deregister fault entity. Broadcast fault entity leave");
            			removeFaultEntity(faultEntity,false);
            		}
            		else
            		{
            			logDebug("FLT","MSG","Entity Fault message from external. Deregister fault entity.");
            			//TODO process fault event
            			removeFaultEntity(faultEntity,true);
            		}
            	}break;
            case SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT:
                    logDebug("FLT","MSG","Entity FAULT message");
                    processFaultEvent(rxMsg->pluginId,eventData,faultEntity);
             break;
          default:
            logDebug("FLT","MSG","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
            break;
        }
    }

    //register a fault client entity
    void FaultServer::RegisterFaultEntity(FaultShmEntry* frp, SAFplus::Handle faultClient,bool needNotify)
    {
    	logDebug("FLT","MSG","Add fault entity with node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
        fsm.createFault(frp,faultClient);
        if(needNotify)
        {
            sendFaultAnnounceMessage(SAFplusI::FaultMessageSendMode::SEND_BROADCAST,faultClient);
        }
    }   
    //Deregister a fault client entity

    void FaultServer::removeFaultEntity(SAFplus::Handle faultClient,bool needNotify)
    {
        //TODO
        logDebug("FLT","MSG","Remove fault entity with node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
    	fsm.faultMap->erase(faultClient);
    	if(needNotify)
    	{
    		logDebug("FLT","MSG","broad cast fault entity leave  with node id [%d] and process Id [%d]",faultClient.getNode(),faultClient.getProcess());
    		sendFaultLeaveMessage(SAFplusI::FaultMessageSendMode::SEND_BROADCAST,faultClient);
    	}
    }
    //Set dependency 
    bool FaultServer::removeDependency(SAFplus::Handle dependencyHandle, SAFplus::Handle faultHandle)
    {
        //TODO
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsm.faultMap->find(faultHandle);
        if (entryPtr == fsm.faultMap->end()) return false; // TODO: raise exception
        FaultShmEntry *fse = &entryPtr->second;
        assert(fse);  // If this fails, something very wrong with the group data structure in shared memory.  TODO, should probably delete it and fail the node
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
    //Remove dependency 
    bool FaultServer::setDependency(SAFplus::Handle dependencyHandle,SAFplus::Handle faultHandle)
    {
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsm.faultMap->find(faultHandle);
        if (entryPtr == fsm.faultMap->end()) return false; // TODO: raise exception
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
    // set name for fault client entity
    void FaultServer::setName(SAFplus::Handle faultHandle, const char* name)
    {
        FaultShmHashMap::iterator entryPtr;
        entryPtr = fsm.faultMap->find(faultHandle);
        if (entryPtr == fsm.faultMap->end()) return; // TODO: raise exception
        SAFplus::FaultShmEntry *fse = &entryPtr->second;
        assert(fse);  // If this fails, something very wrong with the group data structure in shared memory.  TODO, should probably delete it and fail the node
        if (fse) // Name is not meant to change, and not used except for display purposes.  So just change both of them
        {
          strncpy(fse->name,name,FAULT_NAME_LEN);
        }
        else
        {
            //TODO
        }
    }   
    //process a fault event
    void FaultServer::processFaultEvent(SAFplus::FaultPolicy pluginId, FaultEventData fault , SAFplus::Handle faultEntity)
    {
        FaultPolicyMap::iterator it;
        for (it = faultPolicies.begin(); it != faultPolicies.end();it++)
          {
              if(it->first==pluginId)
              {
                  //call Fault plugin to proces fault event
                  FaultPolicyPlugin* pp = dynamic_cast<FaultPolicyPlugin*>(it->second->pluginApi);
                  pp->processFaultEvent(fault,faultEntity);
              }
          }
    }
    //broadcast fault state to all other node

    void FaultServer::sendFaultAnnounceMessage(SAFplusI::FaultMessageSendMode messageMode, SAFplus::Handle handle)
    {
    	logDebug("FLT","FLT","sendFaultAnnounceMessage with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.fault = handle;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_JOIN;
        sndMessage.force = 0;
        sndMessage.faultEntity=INVALID_HDL;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),SAFplusI::FaultMessageSendMode::SEND_BROADCAST);
    }

    void FaultServer::sendFaultLeaveMessage(SAFplusI::FaultMessageSendMode messageMode, SAFplus::Handle handle)
    {
    	logDebug("FLT","FLT","sendFaultAnnounceMessage with node[%d] , process [%d]", handle.getNode(), handle.getProcess());
        FaultMessageProtocol sndMessage;
        sndMessage.fault = handle;
        sndMessage.messageType = FaultMessageTypeT::MSG_ENTITY_LEAVE;
        sndMessage.force = 0;
        sndMessage.faultEntity=INVALID_HDL;
        sndMessage.data.init(SAFplusI::AlarmStateT::ALARM_STATE_INVALID,SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_INVALID,SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_INVALID,SAFplusI::AlarmProbableCauseT::ALARM_ID_INVALID);
        sndMessage.pluginId=SAFplus::FaultPolicy::Undefined;
        sendFaultNotification((void *)&sndMessage,sizeof(FaultMessageProtocol),SAFplusI::FaultMessageSendMode::SEND_BROADCAST);
    }

    void FaultServer::sendFaultNotification(void* data, int dataLength, SAFplusI::FaultMessageSendMode messageMode)
    {
        switch(messageMode)
        {
            case SAFplusI::FaultMessageSendMode::SEND_BROADCAST:
            {
                /* Destination is broadcast address */
                ClIocAddressT iocDest;
                iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
                iocDest.iocPhyAddress.portId      = faultCommunicationPort;
                //logInfo("GMS","MSG","Sending broadcast message");
                try
                {
                    faultMsgServer->SendMsg(iocDest, (void *)data, dataLength, SAFplusI::FAULT_MSG_TYPE);
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
    FaultServer::FaultServer()
    {

    }

};
