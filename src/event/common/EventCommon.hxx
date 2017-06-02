/*
 * EventCommon.hxx
 *
 *  Created on: May 10, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCOMMON_HXX_
#define EVENTCOMMON_HXX_

#include <clLogApi.hxx>
#include <clCommon.hxx>

enum class EventMessageType
    {
        EVENT_CHANNEL_CREATE = 1, EVENT_CHANNEL_SUBSCRIBER, EVENT_CHANNEL_UNSUBSCRIBER, EVENT_CHANNEL_PUBLISHER, EVENT_CHANNEL_CLOSE, EVENT_CHANNEL_UNLINK, EVENT_UNDEFINED
    };

    enum class EventChannelScope
    {
        EVENT_LOCAL_CHANNEL = 1, EVENT_GLOBAL_CHANNEL, EVENT_UNDEFINE
    };

    class EventMessageProtocol
    {
        public:
            EventMessageType messageType;
            EventChannelScope scope;
            SAFplus::Handle clientHandle;
            std::string channelName;
            char                  data[1]; //Not really 1, it will be place on larger memory

            EventMessageProtocol()
            {
                messageType = EventMessageType::EVENT_UNDEFINED;
                scope = EventChannelScope::EVENT_LOCAL_CHANNEL;
                clientHandle = SAFplus::INVALID_HDL;
            }
            void init(SAFplus::Handle handle, std::string evtChannelName, EventChannelScope evtScope,EventMessageType msgType)
            {
                clientHandle=handle;
                messageType = msgType;
                scope = evtScope;
                channelName = evtChannelName;
                data[0]=0;
            }
    };



#endif /* EVENTCOMMON_HXX_ */
