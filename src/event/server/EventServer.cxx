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
#include <inttypes.h>
#include<boost/iostreams/stream_buffer.hpp>
#include<boost/iostreams/stream.hpp>

using namespace SAFplusI;
using namespace SAFplus;
using namespace Rpc;
using namespace rpcEvent;

EventServer::EventServer()
{
	// TODO Auto-generated constructor stub
}

EventServer::~EventServer()
{
	// TODO Auto-generated destructor stub
}
void EventServer::wake(int amt, void* cookie)
{
	Group* g = (Group*) cookie;
	logDebug("EVT", "SERVER", "Group [%" PRIx64 ":%" PRIx64 "] changed", g->handle.id[0], g->handle.id[1]);
	sleep(1);
	if (activeServer != g->getActive())
	{
		if (severHandle == g->getActive())
		{
			//TODO Load global channel from checkpoint
			eventloadchannelFromCheckpoint();
		}
		logInfo("EVT", "DUMP", "Set Active server");
		activeServer = g->getActive();
		esmServer.setActive(activeServer);
	}
}

bool EventServer::eventloadchannelFromCheckpoint()
{
	logInfo("EVT", "DUMP", "---------------------------------");
	Checkpoint::Iterator ibegin = evtCkpt.m_checkpoint.begin();
	Checkpoint::Iterator iend = evtCkpt.m_checkpoint.end();
	for (Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
	{
		BufferPtr curkey = iter->first;
		BufferPtr& curval = iter->second;
		if (curval)
		{
			SAFplus::Rpc::rpcEvent::eventChannelRequest request;
			SAFplus::Checkpoint::KeyValuePair& item = *iter;
			int key = ((eventKey*) (*item.first).data)->id;
			int length = ((eventKey*) (*item.first).data)->length;
			request.ParseFromArray(curval->data,length);
			logDebug("NAME", "GET", "Get object for key [%ld]", key);
			logDebug("NAME", "GET", "Data length [%d]", length);
			logDebug("NAME", "GET", "Get object: Name [%s]", request.channelname().c_str());
			logDebug("NAME", "GET", "Get object: Scope [%d]", request.scope());
			logDebug("NAME", "GET", "Get object: Type [%d]", request.type());

			EventMessageType msgType = EventMessageType(request.type());
			switch (msgType)
			{
			case EventMessageType::EVENT_CHANNEL_CREATE:
				if (1)
				{
					eventChannelCreateHandleRpc(&request);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
				if (1)
				{
					eventChannelSubsHandleRpc(&request);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
				if (1)
				{
					eventChannelUnSubsHandleRpc(&request);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_PUBLISHER:
				if (1)
				{
					eventChannelPubHandleRpc(&request);
				}
				break;
			case EventMessageType::EVENT_CHANNEL_CLOSE:
				if (1)
				{
					eventChannelCloseHandleRpc(&request);
				}
				break;
			default:
				logDebug("EVENT", "MSG", "Unknown message type ... ", request.type());
				break;
			}
		}
	}
	return true;
}

void EventServer::initialize()
{

	logDebug("EVT", "SERVER", "Initialize Event Server");
	severHandle = Handle::create(); // This is the handle for this specific fault server
	numberOfGlobalChannel = 0;
	numberOfLocalChannel = 0;
	esmServer.init();
	logDebug("EVT", "SERVER", "Initialize event checkpoint");
	this->evtCkpt.eventCkptInit();
	group.init(EVENT_GROUP);
	group.setNotification(*this);
	SAFplus::objectMessager.insert(severHandle, this);
	logDebug("EVT", "SERVER", "Register event server [%d] to event group", severHandle.getNode());
	group.registerEntity(severHandle, SAFplus::ASP_NODEADDR, NULL, 0, Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
	activeServer = group.getActive();
	esmServer.setActive(activeServer);
	if (activeServer == severHandle)
	{
		eventloadchannelFromCheckpoint();
	}
	eventMsgServer = &safplusMsgServer;
	if (1)
	{
		logDebug("EVT", "SERVER", "Register rpc stub for Event");
		SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(eventMsgServer, this);
		channel->setMsgSendType(EVENT_REQ_HANDLER_TYPE);
		channel->msgReplyType = EVENT_REPLY_HANDLER_TYPE;
	}

}

int compareChannel(std::string id1, std::string id2)
{
	if (id1 == id2)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

bool EventServer::isPublisher(EventChannel& s, Handle clientHandle)
{
	if (s.eventPubs.empty())
	{
		logDebug("EVT", "MSG", "Event Publisher list is empty");
		return false;
	}
	for (EventPublisherList::iterator iterPub = s.eventPubs.begin(); iterPub != s.eventPubs.end(); iterPub++)
	{
		EventPublisher &evtPub = *iterPub;
		if (clientHandle == evtPub.usr.evtHandle)
		{
			return true;
		}
	}
	return false;
}

bool EventServer::isLocalChannel(std::string channelName)
{
	if (!localChannelList.empty())
	{
		for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.channelName, channelName);
			if (cmp == 0)
			{
				logDebug("EVT", "MSG", "channel [%s] is already exist in local list", channelName.c_str());
				return true;
			}
		}
	}
	else
	{
		logDebug("EVT", "MSG", "Local list is empty");

	}
	logDebug("EVT", "MSG", "channel [%s] is not in local list", channelName.c_str());
	return false;
}

bool EventServer::isGlobalChannel(std::string channelName)
{
	if (!globalChannelList.empty())
	{
		for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.channelName, channelName);
			if (cmp == 0)
			{
				logDebug("EVT", "MSG", "channel [%s] is already exist in global list", channelName.c_str());
				return true;
			}
		}
	}
	else
	{
		logDebug("EVT", "MSG", "global list is empty");
	}
	logDebug("EVT", "MSG", "channel Id [%s] is not in global list", channelName.c_str());
	return false;
}

struct eventChannel_delete_disposer
{
	void operator()(EventChannel *delete_this)
	{
		delete delete_this;
	}
};

void EventServer::sendEventServerMessage(void* data, int dataLength, Handle destHandle)
{
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message to [%d,%d]", destHandle.getNode(), destHandle.getPort());
	try
	{

		eventMsgServer->SendMsg(destHandle, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
	} catch (...) // SAFplus::Error &e)
	{
		logDebug("EVT", "EVENT_ENTITY", "Failed to send.");
	}
}

void EventServer::eventGetActiveServer(const ::SAFplus::Rpc::rpcEvent::NO_REQUEST* request, ::SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event get active server ...");
	SAFplus::Rpc::rpcEvent::Handle *hdl = response->mutable_activeserver();
	hdl->set_id0(activeServer.id[0]);
	hdl->set_id1(activeServer.id[1]);
	logDebug("EVT", "EVENT_ENTITY", "Active Server is [%d,%d] ", activeServer.getNode(), activeServer.getPort());
}

void EventServer::eventChannelRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventChannelRequest* request, ::SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event channel request RPC");
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	EventMessageType msgType = EventMessageType(request->type());
	logDebug("EVT", "MSG", "********** Received event message from client [%d:%d] **********", evtClient.getNode(), evtClient.getPort());
	response->set_saerror(0);
	try
	{
		switch (msgType)

		{
		case EventMessageType::EVENT_CHANNEL_CREATE:
			if (1)
			{
				eventChannelCreateHandleRpc(request);
			}
			break;
		case EventMessageType::EVENT_CHANNEL_SUBSCRIBER:
			if (1)
			{
				eventChannelSubsHandleRpc(request);
			}
			break;
		case EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER:
			if (1)
			{
				eventChannelUnSubsHandleRpc(request);
			}
			break;
		case EventMessageType::EVENT_CHANNEL_PUBLISHER:
			if (1)
			{
				eventChannelPubHandleRpc(request);
			}
			break;
		case EventMessageType::EVENT_CHANNEL_CLOSE:
			if (1)
			{
				eventChannelCloseHandleRpc(request);
			}
			break;
		case EventMessageType::EVENT_CHANNEL_UNLINK:
			if (1)
			{
				logDebug("EVT", "MSG", "Received event channel unlink message from node [%d]", evtClient.getNode());
			}
			break;
		default:
			logDebug("EVENT", "MSG", "Unknown message type ... ", request->type(), evtClient.getNode());
			break;
		}
	} catch (SAFplus::Error& e)
	{
		logDebug("EVT", "MSG", "ERROR : %s , [%d]", e.errStr, e.clError);
		response->set_errstr(e.errStr);
		response->set_saerror(e.clError);
	}
	logDebug("EVT", "MSG", "********** Finish Process event request from client [%d:%d] **********", evtClient.getNode(), evtClient.getPort());

}

void EventServer::eventPublishRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventPublishRequest* request, ::SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event publish request RPC");
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	EventMessageType msgType = EventMessageType(request->type());
	logDebug("EVT", "MSG", "********** Received event message from client [%d:%d] **********", evtClient.getNode(), evtClient.getPort());
	try
	{
		logDebug("EVT", "MSG", "debug 1");
		eventPublishHandleRpc(request);
		logDebug("EVT", "MSG", "debug 2");
	} catch (SAFplus::Error& e)
	{
		logDebug("EVT", "MSG", "debug 3");
		response->set_errstr(e.errStr);
		response->set_saerror(int(e.clError));
		logDebug("EVT", "MSG", "debug 4");
	}
	logDebug("EVT", "MSG", "********** Finish Process event request from client [%d:%d] **********", evtClient.getNode(), evtClient.getPort());

}

void EventServer::createChannelRpc(const eventChannelRequest *request, bool isSub, bool isPub)
{
	EventChannel *channel = new EventChannel();
	channel->scope = EventChannelScope(request->scope());
	channel->subscriberRefCount = 0;
	channel->channelName = request->channelname();
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();

	if (channel->scope == EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		if (!isLocalChannel(channel->channelName))
		{
			logDebug("EVT", "MSG", "Add channel[%s] to local channel list", channel->channelName.c_str());
			localChannelListLock.lock();
			localChannelList.push_back(*channel);
			numberOfLocalChannel += 1;
			logDebug("EVT", "MSG", "Write request message to checkpoint");
			localChannelListLock.unlock();
		}
		else
		{
			logDebug("EVT", "MSG", "Channel is Exist");
			if (isSub == false && isPub == false)
			{
				logDebug("EVT", "MSG", "Ignore request....");
				throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS, "Channel is Exist", __FILE__,
				__LINE__);
			}
		}

		localChannelListLock.lock();
		for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
		{
			EventChannel &s = *iter;
			int cmp = compareChannel(s.channelName, channel->channelName);
			if (cmp == 0)
			{
				if (isSub)
				{
					logDebug("EVT", "MSG", "Register subscriber[%d,%d]", evtClient.getNode(), evtClient.getPort());
					EventSubscriber *sub = new EventSubscriber(evtClient, request->channelname());
					try
					{
						s.addChannelSub(*sub);
					} catch (SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.subscriberRefCount += 1;
				}
				if (isPub)
				{
					logDebug("EVT", "MSG", "Register publisher[%d,%d]", evtClient.getNode(), evtClient.getPort());
					EventPublisher *pub = new EventPublisher(evtClient, request->channelname());
					try
					{
						s.addChannelPub(*pub);
					} catch (SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.publisherRefCount += 1;
				}
				localChannelListLock.unlock();
				return;
			}
		}
		localChannelListLock.unlock();

	}
	else
	{
		if (activeServer == severHandle)
		{

			if (!isGlobalChannel(channel->channelName))
			{
				logDebug("EVT", "MSG", "Add channel[%s] to global channel list", channel->channelName.c_str());
				globalChannelListLock.lock();
				globalChannelList.push_back(*channel);
				logDebug("EVT", "MSG", "write request message to checkpoint with length %d", request->ByteSize());
				evtCkpt.eventCkptCheckPointChannelOpen(request);
				numberOfGlobalChannel += 1;
				globalChannelListLock.unlock();

			}
			else
			{
				logDebug("EVT", "MSG", "Channel is Exist");
				if (isSub == false && isPub == false)
				{
					throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS, "Channel is Exist",
					__FILE__,
					__LINE__);
				}
			}
			globalChannelListLock.lock();
			for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.channelName, request->channelname());
				if (cmp == 0)
				{
					if (isSub)
					{
						logDebug("EVT", "MSG", "Register subscriber[%d,%d] for global channel", evtClient.getNode(), evtClient.getPort());
						EventSubscriber *sub = new EventSubscriber(evtClient, request->channelname());
						try
						{
							s.addChannelSub(*sub);
						} catch (SAFplus::Error& e)
						{
							globalChannelListLock.unlock();
							throw(e);
						}
						logDebug("EVT", "MSG", "write request message to checkpoint with length %d", request->ByteSize());
						evtCkpt.eventCkptCheckPointSubscribeOrPublish(request);
						s.subscriberRefCount += 1;
					}
					if (isPub)
					{
						logDebug("EVT", "MSG", "Register publisher[%d,%d]", evtClient.getNode(), evtClient.getPort());
						EventPublisher *pub = new EventPublisher(evtClient, request->channelname());
						try
						{
							s.addChannelPub(*pub);
						} catch (SAFplus::Error& e)
						{
							globalChannelListLock.unlock();
							throw(e);
						}
						logDebug("EVT", "MSG", "write request message to checkpoint with length %d", request->ByteSize());
						evtCkpt.eventCkptCheckPointSubscribeOrPublish(request);
						s.publisherRefCount += 1;
					}
					//evtCkpt.eventCkptCheckPointSubscribeOrPublish(request,0);
					globalChannelListLock.unlock();
					return;
				}
			}
			globalChannelListLock.unlock();
		}
		else
		{
			logDebug("EVT", "MSG", "forward to active event server");
			//sendEventServerMessage((void*)request,length,activeServer);
		}
	}
}

void EventServer::eventChannelCreateHandleRpc(const eventChannelRequest *request)
{
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_CREATE");
	logDebug("EVT", "MSG", "Channel[%s] -- Event handle [%d,%d]", request->channelname().c_str(), evtClient.getNode(), evtClient.getPort());
	try
	{
		createChannelRpc(request, false, false);
	} catch (SAFplus::Error& e)
	{
		throw(e);

	}
}

void EventServer::eventChannelSubsHandleRpc(const eventChannelRequest *request)
{
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_SUBSCRIBER");
	logDebug("EVT", "MSG", "Channel[%s] -- Event handle [%d,%d]", request->channelname().c_str(), evtClient.getNode(), evtClient.getPort());
	try
	{
		createChannelRpc(request, true, false);
	} catch (SAFplus::Error& e)
	{
		throw(e);

	}
}

void EventServer::eventChannelPubHandleRpc(const eventChannelRequest *request)
{
	//Find channel in local list
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_PUBLISHER");
	logDebug("EVT", "MSG", "Channel[%s] -- Event handle [%d,%d]", request->channelname().c_str(), evtClient.getNode(), evtClient.getPort());
	try
	{
		createChannelRpc(request, false, true);
	} catch (SAFplus::Error& e)
	{
		throw(e);

	}
}

void EventServer::eventChannelUnSubsHandleRpc(const eventChannelRequest *request)
{
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_UNSUBSCRIBER");
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	logDebug("EVT", "MSG", "Channel[%s] -- Event handle [%d,%d]", request->channelname().c_str(), evtClient.getNode(), evtClient.getPort());
	//Find channel in local list
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.channelName, request->channelname());
				if (cmp == 0)
				{
					try
					{
						s.deleteChannelSub(evtClient);
					} catch (SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.subscriberRefCount -= 1;
					logDebug("EVT", "MSG", "Removed subscriber from local event channel[%s]", request->channelname().c_str());
					localChannelListLock.unlock();
					return;
				}
			}
		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Subscriber doesn't exist",
		__FILE__,
		__LINE__);
	}
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();
		if (activeServer == severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.channelName, request->channelname());
					if (cmp == 0)
					{
						try
						{
							s.deleteChannelSub(evtClient);
						} catch (SAFplus::Error& e)
						{
							localChannelListLock.unlock();
							throw(e);
						}
						s.subscriberRefCount -= 1;
						evtCkpt.eventCkptCheckPointUnsubscribeOrUnpublish(request);
						logDebug("EVT", "MSG", "Removed subs from global event channel[%s]", request->channelname().c_str());
						globalChannelListLock.unlock();
						return;
					}
				}
			}
		}
		else
		{

			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED, "Cannot process global request",
			__FILE__,
			__LINE__);
			return;
		}
		// channel not exist
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Subscriber doesn't exist",
		__FILE__,
		__LINE__);

	}
}

void EventServer::eventChannelCloseHandleRpc(const eventChannelRequest *request)
{
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_CLOSE");
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	logDebug("EVT", "MSG", "Channel[%s] -- Event handle [%d,%d]", request->channelname().c_str(), evtClient.getNode(), evtClient.getPort());
	//Find channel in local list
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.channelName, request->channelname());
				if (cmp == 0)
				{
					//TODO
					logDebug("EVT", "MSG", "Remove all subscriber...");
					localChannelList.erase_and_dispose(iter, eventChannel_delete_disposer());
					evtCkpt.eventCkptCheckPointChannelClose(request);
					localChannelListLock.unlock();
					return;
				}
			}

		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Channel doesn't exist",
		__FILE__,
		__LINE__);
	}
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();
		if (activeServer == severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.channelName, request->channelname());
					if (cmp == 0)
					{
						logDebug("EVT", "MSG", "Remove all subscriber... ");
						localChannelList.erase_and_dispose(iter, eventChannel_delete_disposer());
						globalChannelListLock.unlock();
						return;
					}
				}
			}
		}
		else
		{
			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED, "Cannot process global request",
			__FILE__,
			__LINE__);
			return;
		}
		// channel not exist
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Channel doesn't exist",
		__FILE__,
		__LINE__);
	}
	logDebug("EVT", "MSG", "Channel[%s] is removed", request->channelname().c_str());

}

void EventServer::eventPublishHandleRpc(const eventPublishRequest *request)
{
	//Find channel in local list
	logDebug("EVT", "MSG", "Process event message.");
	SAFplus::Handle evtClient;
	evtClient.id[0] = request->clienthandle().id0();
	evtClient.id[1] = request->clienthandle().id1();
	bool isGlobal = false;
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.channelName, request->channelname());
				if (cmp == 0)
				{
					// Check the client in publisher list
					if (!isPublisher(s, evtClient))
					{
						localChannelListLock.unlock();
						logDebug("EVT", "MSG", "EvtClient is not registered as publisher.");
						throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "EvtClient is not registered as publisher",
						__FILE__,
						__LINE__);

					}
					//TODO publish event
					logDebug("EVT", "MSG", "Sending message to all subscriber of channel[%s]", request->channelname().c_str());
					for (EventSubscriberList::iterator iterSub = s.eventSubs.begin(); iterSub != s.eventSubs.end(); iterSub++)
					{
						EventSubscriber &evtSub = *iterSub;
						Handle sub = evtSub.usr.evtHandle;
						//send message to sub
						try
						{
							MsgPool& pool = this->eventMsgServer->getMsgPool();
							Message* msg = pool.allocMsg();
							msg->setAddress(sub);
							if (1)
							{
								boost::iostreams::stream<MessageOStream> mos(msg);
								request->SerializeToOstream(&mos);
							}
							eventMsgServer->SendMsg(msg, SAFplusI::EVENT_MSG_TYPE);
						} catch (Error &e)
						{
							logError("RPC", "REQ", "Serialization Error: %s", e.what());
						}
					}

					localChannelListLock.unlock();
					return;
				}
			}
		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Channel doesn't exist",
		__FILE__,
		__LINE__);

	}
	if (EventChannelScope(request->scope()) == EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();

		if (activeServer == severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.channelName, request->channelname());
					if (cmp == 0)
					{
						//TODO publish event
						logDebug("EVT", "MSG", "Sending message to all subscriber of channel[%s]", request->channelname().c_str());
						for (EventSubscriberList::iterator iterSub = s.eventSubs.begin(); iterSub != s.eventSubs.end(); iterSub++)
						{
							EventSubscriber &evtSub = *iterSub;
							Handle sub = evtSub.usr.evtHandle;
							//send message to sub
							logDebug("EVT", "MSG", "Sending message to subscriber [%d,%d]", sub.getNode(), sub.getPort());
							try
							{
								MsgPool& pool = this->eventMsgServer->getMsgPool();
								Message* msg = pool.allocMsg();
								msg->setAddress(sub);
								if (1)
								{
									boost::iostreams::stream<MessageOStream> mos(msg);
									request->SerializeToOstream(&mos);
								}
								eventMsgServer->SendMsg(msg, SAFplusI::EVENT_MSG_TYPE);
							} catch (Error &e)
							{
								logError("RPC", "REQ", "Serialization Error: %s", e.what());
							}
						}
						globalChannelListLock.unlock();
						return;
					}
				}
			}
		}
		else
		{
			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED, "Cannot process global request",
			__FILE__,
			__LINE__);
			return;
		}
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST, "Channel doesn't exist",
		__FILE__,
		__LINE__);
	}
	// channel not exist

}

