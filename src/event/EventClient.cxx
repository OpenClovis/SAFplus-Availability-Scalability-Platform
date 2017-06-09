/*
 * EventClient.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventClient.hxx"
#include "common/EventChannel.hxx"
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
	return CL_OK;
}

ClRcT EventClient::eventChannelOpen(char* evtChannelName, EventChannelScope scope, SAFplus::Handle &channelHandle)
{
	logDebug("EVT", "EVENT_ENTITY", "Close an event channel");
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelName,scope,EventMessageType::EVENT_CHANNEL_CREATE);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol)+ strlen(evtChannelName),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelClose(char* evtChannelName)
{
	logDebug("EVT", "EVENT_ENTITY", "Close an event channel");
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelName,EventChannelScope::EVENT_UNDEFINE,EventMessageType::EVENT_CHANNEL_CLOSE);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol)+ strlen(evtChannelName),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventChannelUnlink(char* evtChannelName)
{
	logDebug("EVT", "EVENT_ENTITY", "Unlink an event channel");
	EventMessageProtocol sndMessage;
	memset(&sndMessage,0,sizeof(EventMessageProtocol));
	sndMessage.init(clientHandle,evtChannelName,EventChannelScope::EVENT_UNDEFINE,EventMessageType::EVENT_CHANNEL_UNLINK);
	sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol)+ strlen(evtChannelName),INVALID_HDL);
	return CL_OK;
}

ClRcT EventClient::eventPublish(const void *pEventData, int eventDataSize, char* channelName)
{
	logDebug("EVT", "EVENT_ENTITY", "Publish an event");
	char msgPayload[sizeof(EventMessageProtocol)-1 + eventDataSize];
	memset(&msgPayload,0,sizeof(EventMessageProtocol)-1 + eventDataSize);  // valgrind
	EventMessageProtocol *sndMessage = (EventMessageProtocol *)&msgPayload;
	sndMessage->init(clientHandle,channelName,EventChannelScope::EVENT_UNDEFINE,EventMessageType::EVENT_PUBLISH,eventDataSize);
	memcpy(sndMessage->data,(const void*) pEventData,eventDataSize);
	sendEventMessage((void *)sndMessage,sizeof(EventMessageProtocol)+eventDataSize + strlen(channelName),INVALID_HDL);
	return CL_OK;

}


void EventClient::sendEventMessage(void* data, int dataLength,Handle destHandle)
{
	logDebug("EVT", "EVENT_ENTITY", "Sending Event Message");
	Handle destination;
	if (destHandle == INVALID_HDL)
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
	logDebug("EVENT", "EVENT_ENTITY", "Send Event message to local Event Server");
	try
	{
		eventMsgServer->SendMsg(destination, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
	} catch (...) // SAFplus::Error &e)
	{
		logDebug("EVT", "EVENT_ENTITY", "Failed to send.");
	}
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
	switch (msgType)
	{
	case EventMessageType::EVENT_PUBLISH:
		if (1)
		{
			logDebug("EVT", "MSG", "Received event publish message from node [%d]", rxMsg->clientHandle.getNode());
		}
		break;
	default:
		logDebug("EVENT", "MSG", "Unknown message type [%d] from node [%d]", rxMsg->messageType, from.getNode());
		break;
	}
}

