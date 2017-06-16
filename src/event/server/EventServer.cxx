/*
 * EventServer.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventServer.hxx"
#include "EventCommon.hxx"
#include "clCkptApi.hxx"
#include <clLogApi.hxx>
#include <clThreadApi.hxx>
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <clGlobals.hxx>
#include <clObjectMessager.hxx>

using namespace SAFplusI;
using namespace SAFplus;


EventServer::EventServer()
{
	// TODO Auto-generated constructor stub
	//initialize();
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
			logInfo("EVT","DUMP","channel id [%ld]", curkey->data);
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
					eventChannelCreateHandle(data,0);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
				if (1)
				{
					eventChannelSubsHandle(data,0);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
				if (1)
				{
					eventChannelUnSubsHandle(data,0);
				}
				break;
			case EventMessageType::EVENT_PUBLISH:
				if (1)
				{
					eventPublishHandle(data,0);
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
	SAFplus::objectMessager.insert(severHandle,this);
	logDebug("EVT","SERVER", "Register event server [%d] to event group",severHandle.getNode());
	group.registerEntity(severHandle, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
	activeServer=group.getActive();
	eventMsgServer = &safplusMsgServer;
	if(1)
	{
		logDebug("EVT","SERVER", "Register event message server");
		eventMsgServer->RegisterHandler(SAFplusI::EVENT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
		logDebug("EVT","SERVER", "Register event message server with type EVENT_MSG_TYPE ");
	}
	logDebug("EVT","SERVER", "Initialize event checkpoint");
	this->evtCkpt.eventCkptInit();
	//TODO:Dump data from  checkpoint in to channel list
	if(activeServer==severHandle)
	{
		logDebug("EVT","SERVER", "this is active event server");
		//read data from checkpoint to global channel
		logDebug("EVT","SERVER", "Load data from checkpoint to global channel list");
		//		if(!eventloadchannelFromCheckpoint())
		//		{
		//			logError("EVT","SERVER", "Cannot load data from checkpoint to global channel list");
		//			return;
		//		}
	}
}

int compareChannel(uint64_t id1, uint64_t id2)
{
	if (id1==id2)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

bool EventServer::isLocalChannel(uintcw_t channelId)
{
	if (!localChannelList.empty())
	{
		for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.evtChannelId, channelId);
			if (cmp == 0)
			{
				logDebug("EVT", "MSG", "channel Id [%d] is already exist in local list");
				return true;
			}
		}
	}
	else
	{
		logDebug("EVT", "MSG", "local list is empty");

	}
	logDebug("EVT", "MSG", "channel Id [%ld] is not in local list",channelId);
	return false;
}

bool EventServer::isGlobalChannel(uintcw_t channelId)
{
	if (!globalChannelList.empty())
	{
		for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.evtChannelId, channelId);
			if (cmp == 0)
			{
				logDebug("EVT", "MSG", "channel Id [%ld] is already exist in global list");
				return true;
			}
		}
	}
	else
	{
		logDebug("EVT", "MSG", "global list is empty");
	}
	logDebug("EVT", "MSG", "channel Id [%ld] is not in global list",channelId);
	return false;
}

void EventServer::createChannel(EventMessageProtocol *rxMsg,bool isSub,bool isPub,int length)
{
	EventChannel *channel = new EventChannel();
	channel->scope = rxMsg->scope;
	channel->subscriberRefCount=0;
	logDebug("EVT", "MSG", "debug 0");
	channel->evtChannelId = rxMsg->eventChannelId;
	if(channel->scope==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		if(!isLocalChannel(channel->evtChannelId))
		{
			logDebug("EVT", "MSG", "Add channel with Id[%ld] to local channel list",channel->evtChannelId);
			localChannelListLock.lock();
			localChannelList.push_back(*channel);
			numberOfLocalChannel+=1;
			localChannelListLock.unlock();
		}
		else
		{
			logDebug("EVT", "MSG", "Channel is Exist");
		}

		localChannelListLock.lock();
		for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.evtChannelId, channel->evtChannelId);
			if (cmp == 0)
			{
				if(isSub)
				{
					logDebug("EVT", "MSG", "Add event client to local channel list subscriber");
					EventSubscriber *sub=new EventSubscriber(rxMsg->clientHandle,rxMsg->eventChannelId);
					s.addChannelSub(*sub);
					s.subscriberRefCount+=1;
				}
				if(isPub)
				{
					EventPublisher *pub=new EventPublisher(rxMsg->clientHandle,rxMsg->eventChannelId);
					s.addChannelPub(*pub);
					s.publisherRefCount+=1;
				}
				//evtCkpt.eventCkptCheckPointSubscribeOrPublish(rxMsg,0);
				logDebug("EVT", "MSG", "Open channel with channel Id [%ld] successful",rxMsg->eventChannelId);
				localChannelListLock.unlock();
				return ;
			}
		}
		localChannelListLock.unlock();


	}
	else
	{
		if(activeServer==severHandle)
		{

			if(!isGlobalChannel(channel->evtChannelId))
			{
				logDebug("EVT", "MSG", "Add channel with Id[%ld] to global channel list",channel->evtChannelId);
				globalChannelListLock.lock();
				globalChannelList.push_back(*channel);
				//logDebug("EVT", "MSG", "write request message to checkpoint");
				//evtCkpt.eventCkptCheckPointChannelOpen(rxMsg,0);
				numberOfGlobalChannel+=1;
				globalChannelListLock.unlock();

			}
			else
			{
				logDebug("EVT", "MSG", "Channel is Exist");
			}
			globalChannelListLock.lock();
			for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
				if (cmp == 0)
				{
					if(isSub)
					{
						EventSubscriber *sub=new EventSubscriber(rxMsg->clientHandle,rxMsg->eventChannelId);
						s.addChannelSub(*sub);
						s.subscriberRefCount+=1;
					}
					if(isPub)
					{
						EventPublisher *pub=new EventPublisher(rxMsg->clientHandle,rxMsg->eventChannelId);
						s.addChannelPub(*pub);
						s.publisherRefCount+=1;
					}
					//evtCkpt.eventCkptCheckPointSubscribeOrPublish(rxMsg,0);
					logDebug("EVT", "MSG", "Open channel with channel Id [%ld] successful",rxMsg->eventChannelId);
					globalChannelListLock.unlock();
					return ;
				}
			}
			globalChannelListLock.unlock();
		}
		else
		{
			logDebug("EVT", "MSG", "forward to active event server");
			sendEventServerMessage((void*)rxMsg,length,activeServer);
		}
	}
}

void EventServer::eventChannelCreateHandle(EventMessageProtocol *rxMsg,int length)
{
	logDebug("EVT", "MSG", "Received event channel create message with length : [%d].",length);
	createChannel(rxMsg,true,true,length);
}

void EventServer::eventChannelSubsHandle(EventMessageProtocol *rxMsg,int length)
{
	logDebug("EVT", "MSG", "Received event channel create for sub message with length : [%d].",length);
	createChannel(rxMsg,true,false,length);
}


void EventServer::eventChannelPubHandle(EventMessageProtocol *rxMsg,int length)
{
	//Find channel in local list
	logDebug("EVT", "MSG", "Received event channel create for publish message with length : [%d].",length);
	createChannel(rxMsg,false,true,length);

}

void EventServer::eventChannelUnSubsHandle(EventMessageProtocol *rxMsg,int length)
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
			int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
			if (cmp == 0)
			{
				s.deleteChannelSub(rxMsg->clientHandle);
				s.subscriberRefCount-=1;
				logDebug("EVT", "MSG", "remove subs from local event channel [%ld]",rxMsg->eventChannelId);
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
				int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
				if (cmp == 0)
				{
					s.deleteChannelSub(rxMsg->clientHandle);
					s.subscriberRefCount-=1;
					logDebug("EVT", "MSG", "remove subs from global event channel [%ld]",rxMsg->eventChannelId);
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
		sendEventServerMessage((void*)rxMsg,length,activeServer);
		globalChannelListLock.unlock();
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

void EventServer::eventChannelCloseHandle(EventMessageProtocol *rxMsg,int length)
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
			int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
			if (cmp == 0)
			{
				//TODO
				logDebug("EVT", "MSG", "remove all subs from local event channel [%ld]",rxMsg->eventChannelId);
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
				int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
				if (cmp == 0)
				{
					logDebug("EVT", "MSG", "remove all subs from global event channel [%ld]",rxMsg->eventChannelId);
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
		sendEventServerMessage((void*)rxMsg,length,activeServer);
		globalChannelListLock.unlock();
		return;
	}
	// channel not exist
	globalChannelListLock.unlock();
}

void EventServer::eventPublishHandle(EventMessageProtocol *rxMsg,ClWordT msglen)
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
			int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
			if (cmp == 0)
			{
				//TODO publish event
				logDebug("EVT", "MSG", "sending message to all suns of  local event channel [%ld]",rxMsg->eventChannelId);
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
				int cmp = compareChannel(s.evtChannelId, rxMsg->eventChannelId);
				if (cmp == 0)
				{
					//TODO publish event
					logDebug("EVT", "MSG", "sending message to all suns of  global event channel [%ld]",rxMsg->eventChannelId);
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
		sendEventServerMessage((void*)rxMsg,msglen,activeServer);
		globalChannelListLock.unlock();
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
			eventChannelCreateHandle(rxMsg,msglen);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel sub message from node [%d]", from.getNode());
			eventChannelSubsHandle(rxMsg,msglen);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel unsub message from node [%d]", from.getNode());
			eventChannelUnSubsHandle(rxMsg,msglen);
		}
		break;
	case EventMessageType::EVENT_PUBLISH:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event publish message from node [%d]", from.getNode());
			eventPublishHandle(rxMsg,msglen);
		}
		break;
	case EventMessageType::EVENT_CHANNEL_PUBLISHER:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel pubs message from node [%d]", from.getNode());
			eventChannelPubHandle(rxMsg,msglen);


		}
		break;
	case EventMessageType::EVENT_CHANNEL_CLOSE:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event channel close message from node [%d]", from.getNode());
			eventChannelCloseHandle(rxMsg,msglen);
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
