/*
 * EventClient.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCLIENT_HXX_
#define EVENTCLIENT_HXX_

#include <string>

namespace SAFplus
{

enum class EventChannelType
  {
  EVENT_CHANNEL_CREATE=1,
  EVENT_CHANNEL_SUBSCRIBER,
  EVENT_CHANNEL_UNSUBSCRIBER,
  EVENT_CHANNEL_PUBLISHER,
  EVENT_UNDEFINED
  };

enum class EventChannelScope
  {
  EVENT_LOCAL_CHANNEL = 1,
  EVENT_GLOBAL_CHANNEL
  };

class EventData
{


}

class EventMessageProtocol
  {
public:
  SAFplus::EventChannelType     messageType;
  EventChannelScope scope;
  EventData data;
  SAFplus::Handle       clientHandle;

  EventMessageProtocol()
  {
    messageType=SAFplus::EventChannelType::EVENT_UNDEFINED;
    scope=SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL;
    clientHandle=SAFplus::INVALID_HDL;
    }
  };


class EventClient
{
public:
	SAFplus::Handle                   clientHandle;             // handle for identify a event client
	SAFplus::SafplusMsgServer*        eventMsgServer;       // safplus message for send event message to event server
	SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
	SAFplus::Handle                   severHandle;             // handle for identify a event server
	EventClient()
	{
		clientHandle = INVALID_HDL;
		severHandle = INVALID_HDL;
		eventMsgServer = NULL;
		wakeable = NULL;
	}
	virtual ~EventClient();
    ClRcT eventInitialize(clientHandle evtHandle, ClEventCallbacksT * pEvtCallbacks);
    ClRcT eventChannelOpen(std::string evtChannelName,EventChannelType evtChannelOpenFlag, EventChannelScope scope, SAFplus::Handle &channelHandle);
    ClRcT eventChannelClose(SAFplus::Handle channelHandle);
    ClRcT eventChannelUnlink(std::string evtChannelName);
    ClRcT eventPublish(const void *pEventData,int eventDataSize, std::string channelName);
    ClRcT eventPublish(const void *pEventData,int eventDataSize, SAFplus::Handle handle);
};

} /* namespace SAFplus */
#endif /* EVENTCLIENT_HXX_ */
