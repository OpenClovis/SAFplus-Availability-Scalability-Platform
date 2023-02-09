/*
 * EventClient.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCLIENT_HXX_
#define EVENTCLIENT_HXX_

#include <string>
#include "EventCommon.hxx"
#include "clMsgHandler.hxx"
#include "clMsgServer.hxx"
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <FaultSharedMem.hxx>
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
#include <time.h>
#include "clRpcChannel.hxx"
#include "rpcEvent.hxx"
#include "EventSharedMem.hxx"


namespace SAFplus
{


typedef void (*EventCallbackFunction)(std::string,EventChannelScope,std::string,int); // function pointer type
class EventClient:public SAFplus::MsgHandler,public SAFplus::Wakeable
{
public:
	Handle clientHandle;             // handle for identify a event client
	SAFplus::SafplusMsgServer *eventMsgServer;       // safplus message for send event message to event server
	Wakeable* wakeable;             // Wakeable object for change notification
	SAFplus::Handle severHandle;             // handle for identify a event server
	EventCallbackFunction evtCallbacks;
	SAFplus::Rpc::RpcChannel * channel;
	SAFplus::Rpc::rpcEvent::rpcEvent_Stub *service;
	EventSharedMem esm;


	EventClient()
	{
		clientHandle = INVALID_HDL;
		severHandle = INVALID_HDL;
		eventMsgServer = NULL;
		wakeable = NULL;
		evtCallbacks=NULL;
	};

	EventClient(Handle evtHandle)
	{
		clientHandle = evtHandle;
		severHandle = INVALID_HDL;
		eventMsgServer = NULL;
		wakeable = NULL;
		evtCallbacks=NULL;
	};
	virtual ~EventClient();
	uintcw_t eventChannelIdByName(std::string evtChannelName);

//	void  sendEventMessage(void* data, int dataLength);
//	//Create an event channel for pub and subs
//	ClRcT eventChannelOpen(std::string evtChannelName, EventChannelScope scope);
//	//Close an event channel
//	ClRcT eventChannelClose(std::string evtChannelName, EventChannelScope scope);
//	//unlink channel event
//	ClRcT eventChannelUnlink(std::string evtChannelName, EventChannelScope scope);
//	//Publish an event to event channel
//	ClRcT eventPublish(const void *pEventData, int eventDataSize, std::string evtChannelName,EventChannelScope scope);
//	//Subscriber an event channel
//	ClRcT eventChannelSubscriber(std::string evtChannelName, EventChannelScope scope);
//	//Subscriber an event channel
//	ClRcT eventChannelUnSubscriber(std::string evtChannelName, EventChannelScope scope);
//	//Publish an event channel
//	ClRcT eventChannelPublish(std::string evtChannelName, EventChannelScope scope);
//
//	//Create an event channel for pub and subs
//	ClRcT eventChannelOpen(uintcw_t evtChannelId, EventChannelScope scope);
//	//Close an event channel
//	ClRcT eventChannelClose(uintcw_t evtChannelId, EventChannelScope scope);
//	//unlink channel event
//	ClRcT eventChannelUnlink(uintcw_t evtChannelId, EventChannelScope scope);
//	//Publish an event to event channel
//	ClRcT eventPublish(const void *pEventData, int eventDataSize, uintcw_t evtChannelId,EventChannelScope scope);
//	//Subscriber an event channel
//	ClRcT eventChannelSubscriber(uintcw_t evtChannelId, EventChannelScope scope);
//	//Subscriber an event channel
//	ClRcT eventChannelUnSubscriber(uintcw_t evtChannelId, EventChannelScope scope);
//	//Publish an event channel
//	ClRcT eventChannelPublish(uintcw_t evtChannelId, EventChannelScope scope);


	//Initialize an event client
	void eventInitialize(Handle evtHandle,EventCallbackFunction func);

	void wake(int amt,void* cookie);
	//Send event message to event server or active event server
	void sendEventMessage(void* data, int dataLength,Handle destHandle = INVALID_HDL);
	//handle event message
    //virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    virtual void msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie);


    //Event using RPC

	//Create an event channel for pub and subs
	ClRcT eventChannelOpen(std::string evtChannelName, EventChannelScope scope);
	//Close an event channel
	ClRcT eventChannelClose(std::string evtChannelName, EventChannelScope scope);
	//unlink channel event
	ClRcT eventChannelUnlink(std::string evtChannelName, EventChannelScope scope);
	//Publish an event to event channel
	ClRcT eventPublish(std::string pEventData, int eventDataSize, std::string evtChannelName,EventChannelScope scope);
	//Subscriber an event channel
	ClRcT eventChannelSubscriber(std::string evtChannelName, EventChannelScope scope);
	//Subscriber an event channel
	ClRcT eventChannelUnSubscriber(std::string evtChannelName, EventChannelScope scope);
	//Publish an event channel
	ClRcT eventChannelPublish(std::string evtChannelName, EventChannelScope scope);
	ClRcT eventChannelRpc(std::string evtChannelName, EventChannelScope scope,EventMessageType type);
	ClRcT eventPublishRpc(std::string evtChannelName, EventChannelScope scope,EventMessageType type,const std::string data);

};


}
#endif /* EVENTCLIENT_HXX_ */

