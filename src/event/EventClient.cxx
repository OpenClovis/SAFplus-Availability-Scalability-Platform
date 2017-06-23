/*
 * EventClient.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventClient.hxx"
#include "EventChannel.hxx"
#include "clCommon6.h"
#include <clLogApi.hxx>
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
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
#include <clObjectMessager.hxx>
#include <time.h>


using namespace SAFplusI;
using namespace SAFplus;



boost::hash<std::string> channelNameToId;

EventClient::~EventClient()
{
	// TODO Auto-generated destructor stub
}


void EventClient::eventInitialize(Handle evtHandle,EventCallbackFunction func)
{
	logDebug("EVT", "EVENT_ENTITY", "Initialize event Entity");
	clientHandle = evtHandle;
	if (!eventMsgServer)
	{
		eventMsgServer = &safplusMsgServer;
	}
	logDebug("EVT","EVT_SERVER","Register event message server");
	if (1)
	{
		eventMsgServer->RegisterHandler(SAFplusI::EVENT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
	}
	severHandle=getProcessHandle(AMF_IOC_PORT,SAFplus::ASP_NODEADDR);
	evtCallbacks=func;
	//For Rpc
	logDebug("EVT", "EVENT_ENTITY", "Initialize event Rpc client");
	channel = new SAFplus::Rpc::RpcChannel(eventMsgServer, severHandle);
	channel->setMsgType(EVENT_REQ_HANDLER_TYPE,EVENT_REPLY_HANDLER_TYPE);
	service = new SAFplus::Rpc::rpcEvent::rpcEvent_Stub(channel);
	return ;
}
#ifdef USE_MSG
ClRcT EventClient::eventChannelOpen(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_CREATE);
	logDebug("EVT", "EVENT_ENTITY", "create channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}


ClRcT EventClient::eventChannelClose(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_CLOSE);
	logDebug("EVT", "EVENT_ENTITY", "close channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelUnlink(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_UNLINK);
	logDebug("EVT", "EVENT_ENTITY", "Unlink channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);

	return CL_OK;
}

ClRcT EventClient::eventChannelSubscriber(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_SUBSCRIBER);
	logDebug("EVT", "EVENT_ENTITY", "Subscriber channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelUnSubscriber(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER);
	logDebug("EVT", "EVENT_ENTITY", "UnSubscriber channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelPublish(uintcw_t evtChannelId, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_CHANNEL_PUBLISHER);
	logDebug("EVT", "EVENT_ENTITY", "Publisher channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventPublish(const void* pEventData, int eventDataSize,uintcw_t evtChannelId,EventChannelScope scope)
{
	char msgPayload[sizeof(EventMessageProtocol)-1 + eventDataSize];
	memset(&msgPayload,0,sizeof(EventMessageProtocol)-1 + eventDataSize);  // valgrind
	EventMessageProtocol *sndMessage = (EventMessageProtocol *)&msgPayload;
	sndMessage->init(clientHandle,evtChannelId,scope,EventMessageType::EVENT_PUBLISH,eventDataSize);
	memcpy(sndMessage->data,(const void*) pEventData,eventDataSize);
	logDebug("EVT", "EVENT_ENTITY", "Publisher an event to channel[%ld]",sndMessage->eventChannelId);
	sendEventMessage((void *)sndMessage,sizeof(EventMessageProtocol)+eventDataSize ,INVALID_HDL);
	return CL_OK;

}



void EventClient::sendEventMessage(void* data, int dataLength,Handle destHandle)
{
	logDebug("EVT", "EVENT_ENTITY", "Sending Event Message");
	Handle destination;
	if (destHandle != INVALID_HDL)
	{
		destination = destHandle;
		if (destHandle == INVALID_HDL)
		{
			logError("EVT", "EVENT_ENTITY", "No active server... Return");
			return;
		}
	}
	else
	{
		destination = severHandle;
	}
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message (length : %d) to local Event Server [%d] [%d]",dataLength,destination.getNode(),destination.getPort());
	try
	{
		eventMsgServer->SendMsg(destination, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
	} catch (...) // SAFplus::Error &e)
	{
		logDebug("EVT", "EVENT_ENTITY", "Failed to send.");
	}
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message successful");
}

void EventClient::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
	if (msg == NULL)
	{
		logError("EVT", "MSG", "Received NULL message. Ignored fault message");
		return;
	}
	EventMessageProtocol *rxMsg = (EventMessageProtocol *) msg;
	EventMessageType msgType = rxMsg->messageType;
	logDebug("EVT", "MSG", "Received event message from node[%d:%d] with length [%d] data length [%d]", from.getNode(),from.getPort(),msglen,rxMsg->dataLength);
	switch (msgType)
	{
	case EventMessageType::EVENT_PUBLISH:
		if (1)
		{
			//TODO process event message
			this->evtCallbacks(rxMsg->eventChannelId,rxMsg->scope,rxMsg->data,rxMsg->dataLength);
		}
		break;
	default:
		logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, from.getNode());
		break;
	}
}

void EventClient::wake(int amt,void* cookie)
{
	return;
}

uintcw_t EventClient::eventChannelIdByName(std::string evtChannelName)
{
	return static_cast<uintcw_t>(channelNameToId(evtChannelName));
}





//Create an event channel for pub and subs
ClRcT EventClient::eventChannelOpen(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Create event channel [%s] , init event message",evtChannelName.c_str());
	eventChannelOpen(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}



//Close an event channel
ClRcT EventClient::eventChannelClose(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Close event channel[%s]",evtChannelName.c_str());
	eventChannelClose(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
//unlink channel event
ClRcT EventClient::eventChannelUnlink(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Unlink event channel [%s]",evtChannelName.c_str());
	eventChannelUnlink(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
//Publish an event to event channel
ClRcT EventClient::eventPublish(const void *pEventData, int eventDataSize, std::string evtChannelName,EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Publish an event to channel [%s]",evtChannelName.c_str());
	eventPublish(pEventData,eventDataSize,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
//Subscriber an event channel
ClRcT EventClient::eventChannelSubscriber(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	eventChannelSubscriber(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
//Subscriber an event channel
ClRcT EventClient::eventChannelUnSubscriber(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	eventChannelUnSubscriber(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
//Publish an event channel
ClRcT EventClient::eventChannelPublish(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "publish to event channel [%s]",evtChannelName.c_str());
	eventChannelPublish(static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope);
	return CL_OK;
}
#endif

//For event using rpc

void rpcDone(SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	logDebug("EVT", "EVENT_ENTITY", "Event request Done");
}


ClRcT EventClient::eventPublishRpc(std::string evtChannelName, EventChannelScope scope,EventMessageType type,const std::string data)
{

	SAFplus::Rpc::rpcEvent::eventPublishRequest openRequest;
	SAFplus::Rpc::rpcEvent::eventRequestResponse openRequestRes;
	openRequest.set_channelname(evtChannelName);
	openRequest.set_channelid(static_cast<uintcw_t>(channelNameToId(evtChannelName)));
	openRequest.set_scope(int(scope));
	openRequest.set_type(int(EventMessageType::EVENT_CHANNEL_CREATE));
	openRequest.set_data(data);
	SAFplus::Rpc::rpcEvent::Handle *hdl = openRequest.mutable_clienthandle();
	hdl->set_id0(clientHandle.id[0]);
	hdl->set_id1(clientHandle.id[1]);
	google::protobuf::Closure *callback = google::protobuf::NewCallback(&rpcDone, &openRequestRes);
	if(scope==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		service->eventPublishRpcMethod(severHandle,&openRequest,&openRequestRes);
	}
	else
	{
		//TODO
		logDebug("EVT", "EVENT_ENTITY", "Get active server address ... ");
		SAFplus::Rpc::rpcEvent::NO_REQUEST request;
		SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse response;
		service->eventGetActiveServer(severHandle,&request,&response,SAFplus::BLOCK);
		SAFplus::Handle serverAddress;
		serverAddress.id[0]=response.activeserver().id0();
		serverAddress.id[1]=response.activeserver().id1();
		logDebug("EVT", "EVENT_ENTITY", "Active Sever Address : [%d,%d]",serverAddress.getNode(),serverAddress.getPort());
		service->eventPublishRpcMethod(serverAddress,&openRequest,&openRequestRes);
	}
	return CL_OK;
}

ClRcT EventClient::eventChannelRpc(std::string evtChannelName, EventChannelScope scope,EventMessageType type)
{

	SAFplus::Rpc::rpcEvent::eventChannelRequest openRequest;
	SAFplus::Rpc::rpcEvent::eventRequestResponse openRequestRes;
	openRequest.set_channelname(evtChannelName);
	openRequest.set_channelid(static_cast<uintcw_t>(channelNameToId(evtChannelName)));
	openRequest.set_scope(int(scope));
	openRequest.set_type(int(EventMessageType::EVENT_CHANNEL_CREATE));
	SAFplus::Rpc::rpcEvent::Handle *hdl = openRequest.mutable_clienthandle();
	hdl->set_id0(clientHandle.id[0]);
	hdl->set_id1(clientHandle.id[1]);
	if(scope==EventChannelScope::EVENT_LOCAL_CHANNEL)
	{
		service->eventChannelRpcMethod(severHandle,&openRequest,&openRequestRes);
	}
	else
	{
		//TODO
		logDebug("EVT", "EVENT_ENTITY", "Get active server address ... ");
		SAFplus::Rpc::rpcEvent::NO_REQUEST request;
		SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse response;
		service->eventGetActiveServer(severHandle,&request,&response,SAFplus::BLOCK);
		SAFplus::Handle serverAddress;
		serverAddress.id[0]=response.activeserver().id0();
		serverAddress.id[1]=response.activeserver().id1();
		logDebug("EVT", "EVENT_ENTITY", "Active Sever Address : [%d,%d]",serverAddress.getNode(),serverAddress.getPort());
		service->eventChannelRpcMethod(serverAddress,&openRequest,&openRequestRes);
	}
	return CL_OK;
}

//Create an event channel for pub and subs
ClRcT EventClient::eventChannelOpenRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Create event channel [%s] , init event message",evtChannelName.c_str());
	this->eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_CREATE);
	return CL_OK;
}

//Close an event channel
ClRcT EventClient::eventChannelCloseRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Close event channel[%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_CLOSE);
	return CL_OK;
}
//unlink channel event
ClRcT EventClient::eventChannelUnlinkRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Unlink event channel [%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_UNLINK);
	return CL_OK;
}
//Publish an event to event channel
ClRcT EventClient::eventPublishRpc(const void *pEventData, int eventDataSize, std::string evtChannelName,EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Publish an event to channel [%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_PUBLISH);
	return CL_OK;
}
//Subscriber an event channel
ClRcT EventClient::eventChannelSubscriberRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_SUBSCRIBER);
	return CL_OK;
}
//Subscriber an event channel
ClRcT EventClient::eventChannelUnSubscriberRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER);
	return CL_OK;
}
//Publish an event channel
ClRcT EventClient::eventChannelPublishRpc(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "publish to event channel [%s]",evtChannelName.c_str());
	eventChannelRpc(evtChannelName,scope,EventMessageType::EVENT_CHANNEL_PUBLISHER);
	return CL_OK;
}

void EventClient::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
{
	Message* msg = msgHead;
	while(msg)
	{
		assert(msg->firstFragment == msg->lastFragment);  // TODO: This code is only written to handle one fragment.
		MsgFragment* frag = msg->firstFragment;
		SAFplus::Rpc::rpcEvent::eventPublishRequest event;
		event.ParseFromArray(frag->read(0),frag->len);
		ClWordT msgType = event.type();
		this->evtCallbacks(event.channelid(),EventChannelScope(event.scope()),event.data(),0);
		msg = msg->nextMsg;
	}
	msgHead->msgPool->free(msgHead);
}
