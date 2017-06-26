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
		logDebug("EVT","SERVER", "Register rpc stub for Event");
		SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(eventMsgServer, this);
		channel->setMsgType(EVENT_REQ_HANDLER_TYPE, EVENT_REPLY_HANDLER_TYPE);
	}
	logDebug("EVT","SERVER", "Initialize event checkpoint");
	this->evtCkpt.eventCkptInit();
	//TODO:Dump data from  checkpoint in to channel list
	if(activeServer==severHandle)
	{
		logDebug("EVT","SERVER", "This is active event server");
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
				logDebug("EVT", "MSG", "channel Id [%ld] is already exist in local list");
				return true;
			}
		}
	}
	else
	{
		logDebug("EVT", "MSG", "Local list is empty");

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

struct eventChannel_delete_disposer
{
	void operator()(EventChannel *delete_this)
	{
		delete delete_this;
	}
};


//======================================================For RPC call=========================================================


void EventServer::sendEventServerMessage(void* data, int dataLength,Handle destHandle)
{
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message to [%d,%d]",destHandle.getNode(),destHandle.getPort());
	try
	{

		eventMsgServer->SendMsg(destHandle, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
	} catch (...) // SAFplus::Error &e)
	{
		logDebug("EVT", "EVENT_ENTITY", "Failed to send.");
	}
}

void EventServer::eventGetActiveServer(const ::SAFplus::Rpc::rpcEvent::NO_REQUEST* request,
		::SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event get active server ...");
	SAFplus::Rpc::rpcEvent::Handle *hdl = response->mutable_activeserver();
	hdl->set_id0(activeServer.id[0]);
	hdl->set_id1(activeServer.id[1]);
	logDebug("EVT", "EVENT_ENTITY", "Active Server is [%d,%d] ",activeServer.getNode(),activeServer.getPort());
}

void EventServer::eventChannelRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventChannelRequest* request,
		::SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event channel request RPC");
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	EventMessageType msgType = EventMessageType(request->type());
	logDebug("EVT", "MSG", "********** Received event message from client [%d:%d] **********", evtClient.getNode(),evtClient.getPort());
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
	}
	catch(SAFplus::Error& e)
	{
		logDebug("EVT", "MSG", "ERROR : %s , [%d]",e.errStr,e.saError);
		response->set_errstr(e.errStr);
		response->set_saerror(e.clError);
	}
	logDebug("EVT", "MSG", "********** Finish Process event request from client [%d:%d] **********", evtClient.getNode(),evtClient.getPort());

}

void EventServer::eventPublishRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventPublishRequest* request,
		::SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	//TODO: put your code here
	logDebug("EVT", "EVENT_ENTITY", "Receive event publish request RPC");
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	EventMessageType msgType = EventMessageType(request->type());
	logDebug("EVT", "MSG", "********** Received event message from client [%d:%d] **********", evtClient.getNode(),evtClient.getPort());
	try
	{
		eventPublishHandleRpc(request);
	}
	catch(SAFplus::Error& e)
	{
		response->set_errstr(e.errStr);
		response->set_saerror(int(e.saError));
	}
}


void EventServer::createChannelRpc(const eventChannelRequest *request,bool isSub,bool isPub)
{
	EventChannel *channel = new EventChannel();
	channel->scope = EventChannelScope(request->scope());
	channel->subscriberRefCount=0;
	channel->evtChannelId = request->channelid();
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();

	if(channel->scope==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		if(!isLocalChannel(channel->evtChannelId))
		{
			logDebug("EVT", "MSG", "Add channel[%ld] to local channel list",channel->evtChannelId);
			localChannelListLock.lock();
			localChannelList.push_back(*channel);
			numberOfLocalChannel+=1;
			logDebug("EVT", "MSG", "Write request message to checkpoint");
			//evtCkpt.eventCkptCheckPointChannelOpen(request,0);
			localChannelListLock.unlock();
		}
		else
		{
			logDebug("EVT", "MSG", "Channel is Exist");
			if(isSub==false&&isPub==false)
			{
				logDebug("EVT", "MSG", "Ignore request....");
				throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS,"Channel is Exist", __FILE__, __LINE__);
			}
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
					logDebug("EVT", "MSG", "Register subscriber[%d,%d]",evtClient.getNode(),evtClient.getPort());
					EventSubscriber *sub=new EventSubscriber(evtClient,request->channelid());
					try
					{
						s.addChannelSub(*sub);
					}
					catch(SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.subscriberRefCount+=1;
				}
				if(isPub)
				{
					logDebug("EVT", "MSG", "Register publisher[%d,%d]",evtClient.getNode(),evtClient.getPort());
					EventPublisher *pub=new EventPublisher(evtClient,request->channelid());
					try
					{
						s.addChannelPub(*pub);
					}
					catch(SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.publisherRefCount+=1;
				}
				//evtCkpt.eventCkptCheckPointSubscribeOrPublish(request,0);
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
				logDebug("EVT", "MSG", "Add channel[%ld] to global channel list",channel->evtChannelId);
				globalChannelListLock.lock();
				globalChannelList.push_back(*channel);
				//logDebug("EVT", "MSG", "write request message to checkpoint");
				//evtCkpt.eventCkptCheckPointChannelOpen(request,0);
				numberOfGlobalChannel+=1;
				globalChannelListLock.unlock();

			}
			else
			{
				logDebug("EVT", "MSG", "Channel is Exist");
				if(isSub==false&&isPub==false)
				{
					throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS,"Channel is Exist", __FILE__, __LINE__);
				}
			}
			globalChannelListLock.lock();
			for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.evtChannelId, request->channelid());
				if (cmp == 0)
				{
					if(isSub)
					{
						logDebug("EVT", "MSG", "Register subscriber[%d,%d]",evtClient.getNode(),evtClient.getPort());
						EventSubscriber *sub=new EventSubscriber(evtClient,request->channelid());
						try
						{
							s.addChannelSub(*sub);
						}
						catch(SAFplus::Error& e)
						{
							globalChannelListLock.unlock();
							throw(e);
						}
						s.subscriberRefCount+=1;
					}
					if(isPub)
					{
						logDebug("EVT", "MSG", "Register publisher[%d,%d]",evtClient.getNode(),evtClient.getPort());
						EventPublisher *pub=new EventPublisher(evtClient,request->channelid());
						try
						{
							s.addChannelPub(*pub);
						}
						catch(SAFplus::Error& e)
						{
							globalChannelListLock.unlock();
							throw(e);
						}
						s.publisherRefCount+=1;
					}
					//evtCkpt.eventCkptCheckPointSubscribeOrPublish(request,0);
					globalChannelListLock.unlock();
					return ;
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
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_CREATE");
	logDebug("EVT", "MSG", "Channel[%ld] -- Event handle [%d,%d]",request->channelid(),evtClient.getNode(),evtClient.getPort());
	try
	{
		createChannelRpc(request,false,false);
	}
	catch(SAFplus::Error& e)
	{
		throw(e);

	}
}

void EventServer::eventChannelSubsHandleRpc(const eventChannelRequest *request)
{
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_SUBSCRIBER");
	logDebug("EVT", "MSG", "Channel[%ld] -- Event handle [%d,%d]",request->channelid(),evtClient.getNode(),evtClient.getPort());
	try
	{
		createChannelRpc(request,true,false);
	}
	catch(SAFplus::Error& e)
	{
		throw(e);

	}
}


void EventServer::eventChannelPubHandleRpc(const eventChannelRequest *request)
{
	//Find channel in local list
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_PUBLISHER");
	logDebug("EVT", "MSG", "Channel[%ld] -- Event handle [%d,%d]",request->channelid(),evtClient.getNode(),evtClient.getPort());
	try
	{
		createChannelRpc(request,false,true);
	}
	catch(SAFplus::Error& e)
	{
		throw(e);

	}
}

void EventServer::eventChannelUnSubsHandleRpc(const eventChannelRequest *request)
{
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_UNSUBSCRIBER");
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	logDebug("EVT", "MSG", "Channel[%ld] -- Event handle [%d,%d]",request->channelid(),evtClient.getNode(),evtClient.getPort());
	//Find channel in local list
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.evtChannelId, request->channelid());
				if (cmp == 0)
				{
					try
					{
						s.deleteChannelSub(evtClient);
					}
					catch(SAFplus::Error& e)
					{
						localChannelListLock.unlock();
						throw(e);
					}
					s.subscriberRefCount-=1;
					logDebug("EVT", "MSG", "Removed subscriber from local event channel[%ld]",request->channelid());
					localChannelListLock.unlock();
					return ;
				}
			}
		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Subscriber doesn't exist", __FILE__, __LINE__);
	}
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();
		if(activeServer==severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.evtChannelId, request->channelid());
					if (cmp == 0)
					{
						try
						{
							s.deleteChannelSub(evtClient);
						}
						catch(SAFplus::Error& e)
						{
							localChannelListLock.unlock();
							throw(e);
						}
						s.subscriberRefCount-=1;
						logDebug("EVT", "MSG", "Removed subs from global event channel[%ld]",request->channelid());
						globalChannelListLock.unlock();
						return ;
					}
				}
			}
		}
		else
		{

			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED,"Cannot process global request", __FILE__, __LINE__);
			return;
		}
		// channel not exist
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Subscriber doesn't exist", __FILE__, __LINE__);

	}
}


void EventServer::eventChannelCloseHandleRpc(const eventChannelRequest *request)
{
	logDebug("EVT", "MSG", "Process event message with type EVENT_CHANNEL_CLOSE");
	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	logDebug("EVT", "MSG", "Channel[%ld] -- Event handle [%d,%d]",request->channelid(),evtClient.getNode(),evtClient.getPort());
	//Find channel in local list
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.evtChannelId, request->channelid());
				if (cmp == 0)
				{
					//TODO
					logDebug("EVT", "MSG", "Remove all subscriber...");
					localChannelList.erase_and_dispose(iter,eventChannel_delete_disposer());
					localChannelListLock.unlock();
					return ;
				}
			}

		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Channel doesn't exist", __FILE__, __LINE__);
	}
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();
		if(activeServer==severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.evtChannelId, request->channelid());
					if (cmp == 0)
					{
						logDebug("EVT", "MSG", "Remove all subscriber... ");
						localChannelList.erase_and_dispose(iter,eventChannel_delete_disposer());
						globalChannelListLock.unlock();
						return ;
					}
				}
			}
		}
		else
		{
			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED,"Cannot process global request", __FILE__, __LINE__);
			return;
		}
		// channel not exist
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Channel doesn't exist", __FILE__, __LINE__);
	}
	logDebug("EVT", "MSG", "Channel[%ld] is removed",request->channelid());

}

void EventServer::eventPublishHandleRpc(const eventPublishRequest *request)
{
	//Find channel in local list
	logDebug("EVT", "MSG", "Process event message.");

	SAFplus::Handle evtClient;
	evtClient.id[0]=request->clienthandle().id0();
	evtClient.id[1]=request->clienthandle().id1();
	bool isGlobal=false;
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		localChannelListLock.lock();
		if (!localChannelList.empty())
		{
			for (EventChannelList::iterator iter = localChannelList.begin(); iter != localChannelList.end(); iter++)
			{
				EventChannel &s = *iter;
				int cmp = compareChannel(s.evtChannelId, request->channelid());
				if (cmp == 0)
				{
					//TODO publish event
					logDebug("EVT", "MSG", "Sending message to all subscriber of channel[%ld]",request->channelid());
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
								boost::iostreams::stream<MessageOStream>  mos(msg);
								request->SerializeToOstream(&mos);
							}
							eventMsgServer->SendMsg(msg, SAFplusI::EVENT_MSG_TYPE);
						}
						catch (Error &e)
						{
							logError("RPC", "REQ", "Serialization Error: %s", e.what());
						}
					}

					localChannelListLock.unlock();
					return ;
				}
			}
		}
		localChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Channel doesn't exist", __FILE__, __LINE__);

	}
	if(EventChannelScope(request->scope())==EventChannelScope::EVENT_GLOBAL_CHANNEL)
	{
		globalChannelListLock.lock();

		if(activeServer==severHandle)
		{
			if (!globalChannelList.empty())
			{
				for (EventChannelList::iterator iter = globalChannelList.begin(); iter != globalChannelList.end(); iter++)
				{
					EventChannel &s = *iter;
					int cmp = compareChannel(s.evtChannelId, request->channelid());
					if (cmp == 0)
					{
						//TODO publish event
						logDebug("EVT", "MSG", "Sending message to all subscriber of channel[%ld]",request->channelid());
						globalChannelListLock.unlock();
						return ;
					}
				}
			}
		}
		else
		{
			logDebug("EVT", "MSG", "Global event request ... Ignore");
			globalChannelListLock.unlock();
			throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::NOT_IMPLEMENTED,"Cannot process global request", __FILE__, __LINE__);
			return;
		}
		globalChannelListLock.unlock();
		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Channel doesn't exist", __FILE__, __LINE__);
	}
	// channel not exist

}











