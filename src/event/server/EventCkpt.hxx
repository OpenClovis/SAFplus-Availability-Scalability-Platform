/*
 * EventCkpt.hxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCKPT_HXX_
#define EVENTCKPT_HXX_
#include "../common/EventChannel.hxx"
#include "../common/EventCommon.hxx"
#include "clCkptApi.hxx"
#include <string>
#include <clCommon.hxx>

namespace SAFplus
{
    
    class EventCkpt
    {
        public:
            SAFplus::Checkpoint m_checkpoint;

            EventCkpt();
            virtual ~EventCkpt();
            ClRcT eventCkptInit(void);
            ClRcT eventCkptExit(void);
            ClRcT eventCkptSubsDSCreate(std::string *pChannelName,  EventChannelScope channelScope);
            ClRcT eventCkptSubsDSDelete(std::string, EventChannelScope channelScope);
            ClRcT eventCkptCheckPointChannelOpen(EventMessageProtocol* message, int length);
            ClRcT eventCkptCheckPointChannelClose(std::string channelName);
            ClRcT eventCkptCheckPointSubscribe(EventMessageProtocol* message , int length);
            ClRcT eventCkptCheckPointUnsubscribe(EventMessageProtocol* message , int length);

    };

} /* namespace SAFplus */
#endif /* EVENTCKPT_HXX_ */
