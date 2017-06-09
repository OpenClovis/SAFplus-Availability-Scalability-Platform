/*
 * EventServer.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventServer.hxx"
#include "../common/EventCommon.hxx"
#include "clCkptApi.hxx"
#include <clLogApi.hxx>
#include <clThreadApi.hxx>
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <clGlobals.hxx>


using namespace SAFplusI;
using namespace SAFplus;


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

bool EventServer::eventloadchannelFromCheckpoint()
{
	logInfo("EVT","DUMP","---------------------------------");
	SAFplus::Checkpoint::Iterator ibegin = evtCkpt.m_checkpoint.begin();
	SAFplus::Checkpoint::Iterator iend = evtCkpt.m_checkpoint.end();
	for(SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
	{
		BufferPtr curkey = iter->first;
		if (curkey)
		{
			logInfo("EVT","DUMP","channel name [%s]", curkey->data);
		}
		BufferPtr& curval = iter->second;
		if (curval)
		{
			EventMessageProtocol *data = (EventMessageProtocol *) curval->data;
			int length = data->dataLength;
			EventMessageType msgType = data->messageType;
			switch (msgType)
			{
			case EventMessageType::EVENT_CHANNEL_CREATE:
				if (1)
				{
					eventChannelCreateHandle(data);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
				if (1)
				{
					eventChannelSubsHandle(data);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
				if (1)
				{
					eventChannelUnSubsHandle(data);
				}
				break;
			case EventMessageType::EVENT_PUBLISH:
				if (1)
				{
					eventpublish(data,0);
				}
				break;
			default:
				break;
			}		}
	}
	logInfo("EVT","DUMP","---------------------------------");
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
	if(1)
	{
		eventMsgServer->RegisterHandler(SAFplusI::EVENT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
	}
	logDebug("EVT","SERVER", "Initialize event checkpoint");
	this->evtCkpt.eventCkptInit();
	this->evtCkpt.eventCkptInit();
	//TODO:Dump data from  checkpoint in to channel list
	if(activeServer==severHandle)
	{
		logDebug("EVT","SERVER", "this is active event server");
		//read data from checkpoint to global channel
		logDebug("EVT","SERVER", "Load data from checkpoint to global channel list");
		if(!eventloadchannelFromCheckpoint())
		{
			logError("EVT","SERVER", "Cannot load data from checkpoint to global channel list");
			return;
		}
	}
}

int compareChannel(char* name1, char* name2)
{
	if (strcmp(name1,name2))
	{
		return 0;
	} else
	{
		return -1;
	}
}
void EventServer::eventChannelCreateHandle(EventMessageProtocol *rxMsg)
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
		evtCkpt.eventCkptCheckPointChannelOpen(rxMsg,0);
		localChannelListLock.unlock();

	} else
	{
		if (severHandle==activeServer)
		{
			logDebug("EVT", "MSG", "Add to global channel list");
			globalChannelListLock.lock();
			globalChannelList.push_back(*channel);
			evtCkpt.eventCkptCheckPointChannelOpen(rxMsg,0);
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

void EventServer::eventChannelSubsHandle(EventMessageProtocol *rxMsg)
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
				evtCkpt.eventCkptCheckPointSubscribe(rxMsg,0);
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
					evtCkpt.eventCkptCheckPointSubscribe(rxMsg,0);
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

void EventServer::eventChannelUnSubsHandle(EventMessageProtocol *rxMsg)
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

struct eventChannel_delete_disposer
{
	void operator()(EventChannel *delete_this)
	{
		delete delete_this;
	}
};

void EventServer::eventChannelCloseHandle(EventMessageProtocol *rxMsg)
{
	logDebug("EVT", "MSG", "Received event channel close message.");
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
				//TODO
				logDebug("EVT", "MSG", "remove all subs from local event channel [%s]",rxMsg->channelName);
				localChannelList.erase_and_dispose(iter,eventChannel_delete_disposer());
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
					logDebug("EVT", "MSG", "remove all subs from global event channel [%s]",rxMsg->channelName);
					localChannelList.erase_and_dispose(iter,eventChannel_delete_disposer());
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

void EventServer::eventpublish(EventMessageProtocol *rxMsg,ClWordT msglen)
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
				for (EventSubscriberList::iterator iterSub = s.eventSubs.begin(); iterSub != s.eventSubs.end(); iterSub++)
				{
					EventSubscriber &evtSub = *iterSub;
					Handle sub = evtSub.usr.evtHandle;
					//send message to sub
					this->sendEventServerMessage((void*)rxMsg, msglen,sub);
				}

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
			logDebug("EVT", "MSG", "Received event channel create message from node [%d]", from.getNode());
			eventChannelCreateHandle(rxMsg);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel sub message from node [%d]", from.getNode());
			eventChannelSubsHandle(rxMsg);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel unsub message from node [%d]", from.getNode());
			eventChannelUnSubsHandle(rxMsg);
		}
		break;
	case EventMessageType::EVENT_PUBLISH:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event publish message from node [%d]", from.getNode());
			eventpublish(rxMsg,msglen);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_PUBLISHER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel pubs message from node [%d]", from.getNode());
			eventpublish(rxMsg,msglen);

		}
		break;
	case EventMessageType::EVENT_CHANNEL_CLOSE:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel close message from node [%d]", from.getNode());
			eventChannelCloseHandle(rxMsg);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_UNLINK:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel unlink message from node [%d]", from.getNode());
		}
		break;
	default:
		logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, from.getNode());
		break;
	}
}

void EventServer::sendEventServerMessage(void* data, int dataLength,Handle destHandle)
{
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message to local Event Server");
	try
	{
		eventMsgServer->SendMsg(destHandle, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
	} catch (...) // SAFplus::Error &e)
	{
		logDebug("EVT", "EVENT_ENTITY", "Failed to send.");
	}
}
