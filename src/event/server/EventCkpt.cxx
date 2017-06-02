/*
 * EventCkpt::eventCkpt.cxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#include "EventCkpt.hxx"

namespace SAFplus
{
    
    EventCkpt::EventCkpt()
    {
        // TODO Auto-generated constructor stub
    }
    
    EventCkpt::~EventCkpt()
    {
        // TODO Auto-generated destructor stub
    }

    ClRcT EventCkpt::eventCkptInit(void)
    {
    	m_checkpoint.init(EVENT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);
    	m_checkpoint.name = "SAFplus_Event";
        return CL_OK ;
    }



    ClRcT EventCkpt::eventCkptExit(void)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsDSCreate(std::string *pChannelName,  EventChannelScope channelScope)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsDSDelete(std::string, EventChannelScope channelScope)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointChannelOpen(EventMessageProtocol* message, int length)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        m_checkpoint.write(message->channelName,*val);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointChannelClose(std::string channelName)
    {
        m_checkpoint.remove(channelName);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointSubscribe(EventMessageProtocol* message , int length)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        m_checkpoint.write(message->channelName,*val);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointUnsubscribe(EventMessageProtocol* message , int length)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        m_checkpoint.write(message->channelName,*val);
        return CL_TRUE;
    }

} /* namespace SAFplus */





