/*
 * EventCkpt.hxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCKPT_HXX_
#define EVENTCKPT_HXX_
#include "../common/EventChannel.hxx"
namespace SAFplus
{
    
    class EventCkpt
    {
        public:
            SAFplus::Checkpoint eventCkpt;

            EventCkpt();
            virtual ~EventCkpt();
            ClRcT eventCkptInit(void);
            ClRcT eventCkptExit(void);
            ClRcT eventCkptSubsDSCreate(std::string *pChannelName,  SAFplus::EventChannelScope channelScope);
            ClRcT eventCkptSubsDSDelete(std::string, SAFplus::EventChannelScope channelScope);
            ClRcT eventCkptCheckPointChannelOpen(EventMessageProtocol message);
            ClRcT eventCkptCheckPointChannelClose(std::string channelName);
            ClRcT eventCkptCheckPointSubscribe(EventMessageProtocol message);
            ClRcT eventCkptCheckPointUnsubscribe(EventMessageProtocol message);

    };

} /* namespace SAFplus */
#endif /* EVENTCKPT_HXX_ */
