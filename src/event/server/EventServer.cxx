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
        evtCkpt.eventCkptInit();


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

    EventChannel* EventServer::getChannelbyName(std::string channelName)
    {
        localChannelListLock.lock();
        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
        {
            EventChannel &s = *iter;
            int cmp = compareChannel(s.evtChannelName, channelName);
            if (cmp == 0)
            {
                localChannelListLock.unlock();
                return s;
            }
        }
        localChannelListLock.unlock();
        return NULL;
    }


    void EventServer::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        if (msg == NULL)
        {
            logError(FAULT, "MSG", "Received NULL message. Ignored fault message");
            return;
        }
        EventMessageProtocol *rxMsg = (EventMessageProtocol *) msg;
        EventMessageProtocol msgType = rxMsg->messageType;
        switch (msgType)
        {
            case SAFplus::EventMessageType::EVENT_CHANNEL_CREATE:
                if (1)
                {
                    EventChannelScope scope = rxMsg->scope;
                    EventChannel *channel = new EventChannel();
                    channel->evtChannelName = rxMsg->channelName;
                    channel->scope = rxMsg->scope;
                    channel->subscriberRefCount=0;
                    //TODO
                    if (scope == SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL)
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
            case SAFplus::EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
                if (1)
                {
                    //Find channel in local list
                    localChannelListLock.lock();
                    if (localChannelList.empty())
                    {
                        localChannelListLock.unlock();
                        return;
                    }

                    if(severHandle==activeServer)

                    if (scope == SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                        //TODO : find channel in local list
                        // Add handle to subscriber list

                        if (localChannelList.empty())
                        {
                            localChannelListLock.unlock();
                            return;
                        }
                        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
                        {
                            EventChannel &s = *iter;
                            int cmp = compareChannel(s.evtChannelName, channelName);
                            if (cmp == 0)
                            {
                                EventSubscriber *sub=new EventSubscriber();
                                s.addChannelSubs(*sub);
                                s.addChannelSubs(*sub);
                                s.subscriberRefCount+=1;
                                localChannelListLock.unlock();
                                return ;
                            }
                        }
                        localChannelListLock.unlock();
                    }
                    else
                    {
                        if (severHandle==activeServer)
                        {
                            //TODO : find channel in local list
                            // Add handle to subscriber list
                            globalChannelListLock.lock();
                            if (globalChannelList.empty())
                            {
                                globalChannelListLock.unlock();
                                return;
                            }
                            else
                            {
                                for (EventChannelList::iterator iter = globalChannelList.begin(); iter != localChannelList.end(); iter++)
                                {
                                    EventChannel &s = *iter;
                                    int cmp = compareChannel(s.evtChannelName, channelName);
                                    if (cmp == 0)
                                    {
                                        EventSubscriber *sub=new EventSubscriber();
                                        s.addChannelSubs(*sub);
                                        globalChannelListLock.unlock();
                                        return ;
                                    }
                                }
                                globalChannelListLock.unlock();
                                return;
                            }
                        }
                        else
                        {
                            //TODO send to system controller.
                        }
                    }
                }
                break;
            case SAFplus::EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
                if (1)
                {
                    if (scope == SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                        //TODO : find channel in local list
                        // Remove handle to subscriber list
                        //TODO : find channel in local list
                        // Add handle to subscriber list
                        localChannelListLock.lock();
                        if (localChannelList.empty())
                        {
                            localChannelListLock.unlock();
                            return;
                        }
                        for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
                        {
                            EventChannel &s = *iter;
                            int cmp = compareChannel(s.evtChannelName, channelName);
                            if (cmp == 0)
                            {
                                s.deleteChannelSubs(rxMsg->clientHandle);
                                localChannelListLock.unlock();
                                return ;
                            }
                        }
                        localChannelListLock.unlock();

                    } else
                    {
                        //TODO : find channel in local list
                        // Remove handle to subscriber list
                    }

                }
                break;
            case SAFplus::EventMessageType::EVENT_CHANNEL_PUBLISHER:
                if (1)
                {
                    if (scope == SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                        //TODO : find channel in local list
                        // send event to all subscriber
                    } else
                    {
                        //TODO : find channel in local list
                        // send event to all active event server ???
                    }

                }
                break;
            case SAFplus::EventMessageType::EVENT_CHANNEL_CLOSE:
                if (1)
                {
                    if (scope == SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL)
                    {
                        //TODO : find channel in local list
                        // send event to all subscriber
                    } else
                    {
                        //TODO : find channel in local list
                        // send event to all active event server ???
                    }
                }
                break;
            case SAFplus::EventMessageType::EVENT_CHANNEL_UNLINK:
                if (1)
                {
                }
                break;
            default:
                logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, clientHandle.getNode());
                break;
        }
    }

} /* namespace SAFplus */
