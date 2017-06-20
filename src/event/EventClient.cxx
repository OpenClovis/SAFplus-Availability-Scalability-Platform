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

ClRcT EventClient::eventInitialize(Handle evtHandle)
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
	return CL_OK;
}

ClRcT EventClient::eventChannelOpen(std::string evtChannelName, EventChannelScope scope)
{
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	logDebug("EVT", "EVENT_ENTITY", "Create event channel [%s] , init event message",evtChannelName.c_str());
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_CREATE);
	logDebug("EVT", "EVENT_ENTITY", "create channel with id [%ld]",sndMessage.eventChannelId);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelClose(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Close event channel[%s]",evtChannelName.c_str());
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_CLOSE);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelUnlink(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Unlink event channel [%s]",evtChannelName.c_str());
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_UNLINK);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelSubscriber(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_SUBSCRIBER);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelUnSubscriber(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "subscriber to  event channel [%s]",evtChannelName.c_str());
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_UNSUBSCRIBER);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelPublish(std::string evtChannelName, EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "publish to event channel [%s]",evtChannelName.c_str());
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,static_cast<uintcw_t>(channelNameToId(evtChannelName)),scope,EventMessageType::EVENT_CHANNEL_PUBLISHER);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventPublish(const void* pEventData, int eventDataSize, std::string channelName,EventChannelScope scope)
{
	logDebug("EVT", "EVENT_ENTITY", "Publish an event to channel [%s]",channelName.c_str());
	char msgPayload[sizeof(EventMessageProtocol)-1 + eventDataSize];
	memset(&msgPayload,0,sizeof(EventMessageProtocol)-1 + eventDataSize);  // valgrind
	EventMessageProtocol *sndMessage = (EventMessageProtocol *)&msgPayload;
	sndMessage->init(clientHandle,static_cast<uintcw_t>(channelNameToId(channelName)),scope,EventMessageType::EVENT_PUBLISH,eventDataSize);
	memcpy(sndMessage->data,(const void*) pEventData,eventDataSize);
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
			char* data = (char*)malloc(rxMsg->dataLength);
		    memcpy(data,rxMsg->data,rxMsg->dataLength);
			logDebug("EVT", "MSG", "Event data [%s]", data);
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
