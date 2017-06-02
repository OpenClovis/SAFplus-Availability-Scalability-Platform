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

        logDebug("EVT","SERVER", "Register event group");
        group.registerEntity(severHandle, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
        activeServer=group.getActive();

        logDebug("EVT","SERVER", "Initialize event checkpoint");
        this->evtCkpt.eventCkptInit();


        //TODO:Dump data from  checkpoint in to channel list
        if(activeServer==severHandle)
        {
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
                    EventChannelScope scope = rxMsg->scope;
                    EventChannel *channel = new EventChannel();
                    channel->evtChannelName = rxMsg->channelName;
                    channel->scope = rxMsg->scope;
                    channel->subscriberRefCount=0;
                    //TODO
                    if (scope == EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                        localChannelListLock.lock();
                        localChannelList.push_back(*channel);
                        numberOfLocalChannel+=1;
                        localChannelListLock.unlock();

                    } else
                    {
                        if (severHandle==activeServer)
                        {
                            globalChannelListLock.lock();
                            globalChannelList.push_back(*channel);
                            numberOfGlobalChannel+=1;
                            globalChannelListLock.unlock();
                        }
                        else
                        {
                            //TODO Send to system controller node
                        }
                    }
                }
                break;
            case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
                if (1)
                {
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
                                EventSubscriber *sub=new EventSubscriber(rxMsg->clientHandle,rxMsg->channelName);
                                s.addChannelSub(*sub);
                                s.subscriberRefCount+=1;
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
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
                        return;
                    }
                    globalChannelListLock.unlock();
                    // channel not exist
                }
                break;
            case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
                if (1)
                {
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
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
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
                                    globalChannelListLock.unlock();
                                    return ;
                                }
                            }
                        }
                    }
                    else
                    {
                        //send to active server
                        return;
                    }
                    globalChannelListLock.unlock();
                    // channel not exist

                }
                break;
            case EventMessageType::EVENT_CHANNEL_CLOSE:
                if (1)
                {

                }
                break;
            case EventMessageType::EVENT_CHANNEL_UNLINK:
                if (1)
                {
                }
                break;
            default:
                logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, from.getNode());
                break;
        }
    }

} /* namespace SAFplus */
