/*
 * EventServer.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTSERVER_HXX_
#define EVENTSERVER_HXX_


#include <EventChannel.hxx>
#include <EventCommon.hxx>
#include <EventCkpt.hxx>
#include <clGroupApi.hxx>
#include <string>
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
#include <clCustomization.hxx>
#include <clNameApi.hxx>
#include <EventSharedMem.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <time.h>
#include <rpcEvent.hxx>



namespace SAFplus
{

class EventServer:public SAFplus::MsgHandler,public SAFplus::Wakeable, public SAFplus::Rpc::rpcEvent::rpcEvent
{

public:
	SAFplus::Handle                   severHandle;             // handle for identify a event client
	EventSharedMem esmServer;
	SAFplus::SafplusMsgServer*        eventMsgServer;       // safplus message for send event message to event server
	SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
	SAFplus::Handle activeServer;
	EventChannelList localChannelList; //? contain local channel
	EventChannelList globalChannelList; //? contain global channel
	SAFplus::Mutex localChannelListLock;
	SAFplus::Mutex globalChannelListLock;
	int numberOfGlobalChannel;
	int numberOfLocalChannel;
	//? fault server group
	SAFplus::Group group; //? event sever group
	EventCkpt evtCkpt;
	SAFplus::Rpc::RpcChannel *channel;

	EventServer();
	virtual ~EventServer();
	void initialize();
	void wake(int amt,void* cookie=NULL);
	bool isPublisher(EventChannel& s , Handle clientHandle);
	bool isLocalChannel(const std::string&  channelName);
	bool isGlobalChannel(const std::string&  channelName);
	void sendEventServerMessage(const void* data,const int& dataLength,const Handle& destHandle);
	bool eventloadchannelFromCheckpoint();
	//RPC call
	void eventChannelRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventChannelRequest* request,
			::SAFplus::Rpc::rpcEvent::eventRequestResponse* response);
	void eventPublishRpcMethod(const ::SAFplus::Rpc::rpcEvent::eventPublishRequest* request,
			::SAFplus::Rpc::rpcEvent::eventRequestResponse* response);
	void eventGetActiveServer(const ::SAFplus::Rpc::rpcEvent::NO_REQUEST* request,
	                              ::SAFplus::Rpc::rpcEvent::eventGetActiveServerResponse* response);
	void eventChannelCreateHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventChannelCloseHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventChannelSubsHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventChannelPubHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventChannelUnPubHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventChannelUnSubsHandleRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request);
	void eventPublishHandleRpc(const SAFplus::Rpc::rpcEvent::eventPublishRequest *request);
	void createChannelRpc(const SAFplus::Rpc::rpcEvent::eventChannelRequest *request,const bool& isCreate = true,const bool& isSub = true,const bool& isPub = true,const bool& isWrite = true);
};

} /* namespace SAFplus */
#endif /* EVENTSERVER_HXX_ */
