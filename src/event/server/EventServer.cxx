/*
 * EventServer.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventServer.hxx"
#include "../common/EventCommon.hxx"

namespace SAFplus
{

    EventServer::EventServer()
    {
        // TODO Auto-generated constructor stub
    	initialize();
    }

    EventServer::~EventServer()
    {
        // TODO Auto-generated destructor stub
    }
    void EventServer::wake(int amt,void* cookie)
    {

         Group* g = (Group*) cookie;
         logDebug("EVT","SERVER", "Group [%" PRIx64 ":%" PRIx64 "] changed", g->handle.id[0],g->handle.id[1]);
         activeServer = g->getActive();
    }

    bool EventServer::eventloadGlobalchannel()
    {
        return true ;
    }
    bool EventServer::eventloadLocalchannel()
    {
        return true ;
    }

    void EventServer::initialize()
    {
        logDebug("EVT","SERVER", "Initialize Event Server");
        severHandle = Handle::create();  // This is the handle for this specific fault server
        numberOfGlobalChannel=0;
        numberOfLocalChannel=0;
        group.init(EVENT_GROUP);
        group.setNotification(*this);
        logDebug("EVT","SERVER", "Register event server [] to event group");
        group.registerEntity(severHandle, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
        activeServer=group.getActive();
        logDebug("EVT","SERVER", "Initialize event checkpoint");
        this->evtCkpt.eventCkptInit();
        //TODO:Dump data from  checkpoint in to channel list
        if(activeServer==severHandle)
        {
        	logDebug("EVT","SERVER", "this is active event server");
            //read data from checkpoint to global channel
            logDebug("EVT","SERVER", "Load data from checkpoint to global channel list");
            if(!eventloadGlobalchannel())
            {
                logError("EVT","SERVER", "Cannot load data from checkpoint to global channel list");
                return;
            }
        }
        //read data from checkpoint to local channel
        if(!eventloadLocalchannel())
        {
            logError("EVT","SERVER", "Cannot load data from checkpoint to local channel list");
            return;
        }
    }

    int compareChannel(std::string name1, std::string name2)
    {
        if (name1 == name2)
        {
            return 0;
        } else
        {
            return -1;
        }
    }

    void EventServer::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        if (msg == NULL)
        {
            logError("EVT", "MSG", "Received NULL message. Ignored fault message");
            return;
        }
        EventMessageProtocol *rxMsg = (EventMessageProtocol *) msg;
        EventMessageType msgType = rxMsg->messageType;
        switch (msgType)
        {
            case EventMessageType::EVENT_CHANNEL_CREATE:
                if (1)
                {
                    logDebug("EVT", "MSG", "Received event channel create message.");
                    EventChannelScope scope = rxMsg->scope;
                    EventChannel *channel = new EventChannel();
                    channel->evtChannelName = rxMsg->channelName;
                    channel->scope = rxMsg->scope;
                    channel->subscriberRefCount=0;
                    //TODO
                    if (scope == EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                    	logDebug("EVT", "MSG", "Add to local channel list");
                        localChannelListLock.lock();
                        localChannelList.push_back(*channel);
                        numberOfLocalChannel+=1;
                        localChannelListLock.unlock();

                    } else
                    {
                        if (severHandle==activeServer)
                        {
                        	logDebug("EVT", "MSG", "Add to global channel list");
                            globalChannelListLock.lock();
                            globalChannelList.push_back(*channel);
                            numberOfGlobalChannel+=1;
                            globalChannelListLock.unlock();
                        }
                        else
                        {
                            //TODO Send to system controller node
                        	logDebug("EVT", "MSG", "forward to active event server");
                        }
                    }
                }
                break;
            case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
                if (1)
                {
                    //Find channel in local list
                    logDebug("EVT", "MSG", "Received event channel subs message.");
                    bool isGlobal=false;
                    localChannelListLock.lock();
                    if (!localChannelList.empty())
                    {
                        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
                        {
                            EventChannel &s = *iter;
                            int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                            if (cmp == 0)
                            {
                                EventSubscriber *sub=new EventSubscriber(rxMsg->clientHandle,rxMsg->channelName);
                                s.addChannelSub(*sub);
                                s.subscriberRefCount+=1;
                                logDebug("EVT", "MSG", "Add subs to local event channel [%s]",rxMsg->channelName);
                                localChannelListLock.unlock();
                                return ;
                            }
                        }
                    }
                    localChannelListLock.unlock();
                    globalChannelListLock.lock();
                    if(activeServer==severHandle)
                    {
                        if (!globalChannelList.empty())
                        {
                            for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
                            {
                                EventChannel &s = *iter;
                                int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                                if (cmp == 0)
                                {
                                    EventSubscriber *sub=new EventSubscriber(rxMsg->clientHandle,rxMsg->channelName);
                                    s.addChannelSub(*sub);
                                    s.subscriberRefCount+=1;
                                    logDebug("EVT", "MSG", "Add subs to global event channel [%s]",rxMsg->channelName);
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
                    	logDebug("EVT", "MSG", "Forward to active server");
                        return;
                    }
                    globalChannelListLock.unlock();
                    // channel not exist
                }
                break;
            case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
                if (1)
                {
                    logDebug("EVT", "MSG", "Received event channel unSubs create message.");
                    //Find channel in local list
                    bool isGlobal=false;
                    localChannelListLock.lock();
                    if (!localChannelList.empty())
                    {
                        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
                        {
                            EventChannel &s = *iter;
                            int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                            if (cmp == 0)
                            {
                                s.deleteChannelSub(rxMsg->clientHandle);
                                s.subscriberRefCount-=1;
                                logDebug("EVT", "MSG", "remove subs from local event channel [%s]",rxMsg->channelName);
                                localChannelListLock.unlock();
                                return ;
                            }
                        }
                    }
                    localChannelListLock.unlock();
                    globalChannelListLock.lock();

                    if(activeServer==severHandle)
                    {
                        if (!globalChannelList.empty())
                        {
                            for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
                            {
                                EventChannel &s = *iter;
                                int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                                if (cmp == 0)
                                {
                                    s.deleteChannelSub(rxMsg->clientHandle);
                                    s.subscriberRefCount-=1;
                                    logDebug("EVT", "MSG", "remove subs from global event channel [%s]",rxMsg->channelName);
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
                    	logDebug("EVT", "MSG", "Forward to active server");

                        return;
                    }
                    // channel not exist
                    globalChannelListLock.unlock();
                }
                break;
            case EventMessageType::EVENT_CHANNEL_PUBLISHER:
                if (1)
                {
                    //Find channel in local list
                    logDebug("EVT", "MSG", "Received event  message.");
                    bool isGlobal=false;
                    localChannelListLock.lock();
                    if (!localChannelList.empty())
                    {
                        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
                        {
                            EventChannel &s = *iter;
                            int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                            if (cmp == 0)
                            {
                                //TODO publish event
                            	logDebug("EVT", "MSG", "sending message to all suns of  local event channel [%s]",rxMsg->channelName);
                                localChannelListLock.unlock();
                                return ;
                            }
                        }
                    }
                    localChannelListLock.unlock();
                    globalChannelListLock.lock();
                    if(activeServer==severHandle)
                    {
                        if (!globalChannelList.empty())
                        {
                            for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
                            {
                                EventChannel &s = *iter;
                                int cmp = compareChannel(s.evtChannelName, rxMsg->channelName);
                                if (cmp == 0)
                                {
                                    //TODO publish event
                                	logDebug("EVT", "MSG", "sending message to all suns of  global event channel [%s]",rxMsg->channelName);
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
                    	logDebug("EVT", "MSG", "Forward to active server");
                        return;
                    }
                    globalChannelListLock.unlock();
                    // channel not exist

                }
                break;
            case EventMessageType::EVENT_CHANNEL_CLOSE:
                if (1)
                {
                    logDebug("EVT", "MSG", "Received event channel close message.");

                }
                break;
            case EventMessageType::EVENT_CHANNEL_UNLINK:
                if (1)
                {
                    logDebug("EVT", "MSG", "Received event channel unlink message.");
                }
                break;
            default:
                logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, from.getNode());
                break;
        }
    }

} /* namespace SAFplus */
