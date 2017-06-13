/*
 * EventClient.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCLIENT_HXX_
#define EVENTCLIENT_HXX_

#include <string>
#include "common/EventCommon.hxx"
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

namespace SAFplus
{
class EventClient:public SAFplus::MsgHandler,public SAFplus::Wakeable
{
public:
	Handle clientHandle;             // handle for identify a event client
	SAFplus::SafplusMsgServer *eventMsgServer;       // safplus message for send event message to event server
	Wakeable* wakeable;             // Wakeable object for change notification
	Handle severHandle;             // handle for identify a event server

	EventClient()
	{
		clientHandle = INVALID_HDL;
		severHandle = INVALID_HDL;
		eventMsgServer = NULL;
		wakeable = NULL;
	};
	virtual ~EventClient();

	void  sendEventMessage(void* data, int dataLength);
	//Initialize an event client
	ClRcT eventInitialize(Handle evtHandle);
	//Create an event channel
	ClRcT eventChannelOpen(char* evtChannelName, EventChannelScope scope, SAFplus::Handle &channelHandle);
	//Close an event channel
	ClRcT eventChannelClose(char* evtChannelName);
	ClRcT eventChannelUnlink(char* evtChannelName);
	//Publish an event to event channel
	ClRcT eventPublish(const void *pEventData, int eventDataSize, char* channelName);
	//Send event message to event server or active event server
	void sendEventMessage(void* data, int dataLength,Handle destHandle = INVALID_HDL);
	ClRcT eventChannelSubscriber(char* evtChannelName);
	ClRcT eventChannelPublish(char* evtChannelName);

    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);





};


}
#endif /* EVENTCLIENT_HXX_ */

