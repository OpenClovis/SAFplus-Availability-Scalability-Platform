/*
 * EventClient.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include <EventClient.hxx>
#include <EventChannel.hxx>
#include <clCommon6.h>
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

void EventClient::eventInitialize(Handle evtHandle, EventCallbackFunction func)
{
	logDebug("EVT", "EVENT_ENTITY", "Initialize event Entity");
	clientHandle = evtHandle;
	esm.init();
	if (!eventMsgServer)
	{
		eventMsgServer = &safplusMsgServer;
	}
	logDebug("EVT", "EVT_SERVER", "Register event message server");
	if (1)
	{
		eventMsgServer->RegisterHandler(SAFplusI::EVENT_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
	}
	severHandle = getProcessHandle(AMF_IOC_PORT, SAFplus::ASP_NODEADDR);
	evtCallbacks = func;
	//For Rpc
	logDebug("EVT", "EVENT_ENTITY", "Initialize event Rpc client");
	channel = new SAFplus::Rpc::RpcChannel(eventMsgServer, severHandle);
	channel->setMsgReplyType(EVENT_REPLY_HANDLER_TYPE);
	channel->msgSendType = EVENT_REQ_HANDLER_TYPE;
	service = new SAFplus::Rpc::rpcEvent::rpcEvent_Stub(channel);
	return;
}

//For event using rpc

void rpcDone(SAFplus::Rpc::rpcEvent::eventRequestResponse* response)
{
	logDebug("EVT", "EVENT_ENTITY", "Event request Done");
}

ClRcT EventClient::eventPublishRpc(std::string evtChannelName, EventChannelScope scope, EventMessageType type, const std::string data)
{

	SAFplus::Rpc::rpcEvent::eventPublishRequest openRequest;
	SAFplus::Rpc::rpcEvent::eventRequestResponse openRequestRes;
	openRequest.set_channelname(evtChannelName);
	openRequest.set_scope(int(scope));
	openRequest.set_type(int(type));
	openRequest.set_data(data);
	SAFplus::Rpc::rpcEvent::Handle *hdl = openRequest.mutable_clienthandle();
	hdl->set_id0(clientHandle.id[0]);
	hdl->set_id1(clientHandle.id[1]);
	try
	{
		google::protobuf::Closure *callback = google::protobuf::NewCallback(&rpcDone, &openRequestRes);
		if (scope == EventChannelScope::EVENT_LOCAL_CHANNEL)
		{
			service->eventPublishRpcMethod(severHandle, &openRequest, &openRequestRes);
			if (openRequestRes.saerror() != 0)
			{
				logDebug("EVT", "EVENT_ENTITY", "Error : %s ... ", openRequestRes.errstr().c_str());
				return openRequestRes.saerror();
			}
		}
		else
		{
			//TODO
			logDebug("EVT", "EVENT_ENTITY", "Get active server address ... ");
//			SAFplus::Rpc::rpcEvent::NO_REQUEST request;
//			SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse response;
//			service->eventGetActiveServer(severHandle, &request, &response);
//			SAFplus::Handle serverAddress;
//			serverAddress.id[0] = response.activeserver().id0();
//			serverAddress.id[1] = response.activeserver().id1();
			SAFplus::Handle serverAddress;
			serverAddress=esm.getActive();
			logDebug("EVT", "EVENT_ENTITY", "Active Sever Address : [%d,%d]", serverAddress.getNode(), serverAddress.getPort());
                        if (serverAddress == INVALID_HDL)
                        {
                           logWarning("EVT", "EVENT_ENTITY", "Active event server is not available. Try again later");
                           return CL_ERR_TRY_AGAIN;
                        }
			service->eventPublishRpcMethod(serverAddress, &openRequest, &openRequestRes, SAFplus::BLOCK);
			if (openRequestRes.saerror() != 0)
			{
				logDebug("EVT", "EVENT_ENTITY", "Error : %s ... ", openRequestRes.errstr().c_str());
				return openRequestRes.saerror();
			}
		}
	} catch (Error &e)
	{
		// throw;
		if (e.clError == Error::DOES_NOT_EXIST)
		{
				logWarning("EVT", "EVENT_ENTITY", "Active event server's fault state is DOWN. Ignore event publish request.");
				return CL_ERR_INVALID_STATE;
		}
		return CL_ERR_NOT_EXIST;        //Fault manager has no information about destination
	}
	return CL_OK;
}

ClRcT EventClient::eventChannelRpc(std::string evtChannelName, EventChannelScope scope, EventMessageType type)
{

	SAFplus::Rpc::rpcEvent::eventChannelRequest openRequest;
	SAFplus::Rpc::rpcEvent::eventRequestResponse openRequestRes;
	openRequest.set_channelname(evtChannelName);
	openRequest.set_scope(int(scope));
	openRequest.set_type(int(type));
	SAFplus::Rpc::rpcEvent::Handle *hdl = openRequest.mutable_clienthandle();
	hdl->set_id0(clientHandle.id[0]);
	hdl->set_id1(clientHandle.id[1]);
	try
	{
		if (scope == EventChannelScope::EVENT_LOCAL_CHANNEL)
		{
			service->eventChannelRpcMethod(severHandle, &openRequest, &openRequestRes, SAFplus::BLOCK);
			logDebug("EVT", "EVENT_ENTITY", "Return code [%d] ... ", openRequestRes.saerror());
			if (openRequestRes.saerror() != 0)
			{
				logDebug("EVT", "EVENT_ENTITY", "Error : %s ... ", openRequestRes.errstr().c_str());
				return openRequestRes.saerror();
			}
		}
		else
		{
			//TODO
			logDebug("EVT", "EVENT_ENTITY", "Get active server address ... ");
//			SAFplus::Rpc::rpcEvent::NO_REQUEST request;
//			SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse response;
//			service->eventGetActiveServer(severHandle, &request, &response, SAFplus::BLOCK);
//			serverAddress.id[0] = response.activeserver().id0();
//			serverAddress.id[1] = response.activeserver().id1();
			SAFplus::Handle serverAddress;
			serverAddress=esm.getActive();
			logDebug("EVT", "EVENT_ENTITY", "Active Server Address : [%d,%d]", serverAddress.getNode(), serverAddress.getPort());
                        if (serverAddress == INVALID_HDL)
                        {
                           logWarning("EVT", "EVENT_ENTITY", "Active event server is not available. Try again later");
                           return CL_ERR_TRY_AGAIN;
                        }
			service->eventChannelRpcMethod(serverAddress, &openRequest, &openRequestRes, SAFplus::BLOCK);
			logDebug("EVT", "EVENT_ENTITY", "Return code [%d] ... ", openRequestRes.saerror());
			if (openRequestRes.saerror() != 0)
			{
				logDebug("EVT", "EVENT_ENTITY", "Error : %s ... ", openRequestRes.errstr().c_str());
				return openRequestRes.saerror();
			}
		}

	} catch (...)
	{
		throw;
	}
	return CL_OK;
}

//Create an event channel for pub and subs
ClRcT EventClient::eventChannelOpen(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Create event channel [%s] , init event message", evtChannelName.c_str());
	return this->eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_CREATE);	
}

//Close an event channel
ClRcT EventClient::eventChannelClose(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Close event channel[%s]", evtChannelName.c_str());
	return eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_CLOSE);
	
}
//unlink channel event
ClRcT EventClient::eventChannelUnlink(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Unlink event channel [%s]", evtChannelName.c_str());
	return eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_UNLINK);
	
}
//Publish an event to event channel
ClRcT EventClient::eventPublish(std::string pEventData, int eventDataSize, std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Publish an event to channel [%s]", evtChannelName.c_str());
	return eventPublishRpc(evtChannelName, scope, EventMessageType::EVENT_PUBLISH, pEventData);
	
}
//Subscribe an event channel
ClRcT EventClient::eventChannelSubscribe(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Subscribe to  event channel [%s]", evtChannelName.c_str());
	return eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_SUBSCRIBER);
	
}
//Subscribe an event channel
ClRcT EventClient::eventChannelUnsubscribe(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Unsubscribe to  event channel [%s]", evtChannelName.c_str());
	return eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER);

}
//Publish an event channel
ClRcT EventClient::eventChannelPublish(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "publisher to event channel [%s]", evtChannelName.c_str());
	return eventChannelRpc(evtChannelName, scope, EventMessageType::EVENT_CHANNEL_PUBLISHER);
	
}

void EventClient::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
{
	Message* msg = msgHead;
	while (msg)
	{
		assert(msg->firstFragment == msg->lastFragment);  // TODO: This code is only written to handle one fragment.
		MsgFragment* frag = msg->firstFragment;
		SAFplus::Rpc::rpcEvent::eventPublishRequest event;
		event.ParseFromArray(frag->read(0), frag->len);
		ClWordT msgType = event.type();
		this->evtCallbacks(event.channelname(), EventChannelScope(event.scope()), event.data(), 0);
		msg = msg->nextMsg;
	}
	msgHead->msgPool->free(msgHead);
}

void EventClient::wake(int amt, void* cookie)
{
	return;
}
