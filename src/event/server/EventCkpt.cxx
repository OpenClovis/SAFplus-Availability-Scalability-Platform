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
        eventCkpt.init(EVENT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);
        eventCkpt.name = "SAFplus_Event";
        return CL_OK ;
    }



    ClRcT EventCkpt::eventCkptExit(void)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsDSCreate(std::string *pChannelName,  SAFplus::EventChannelScope channelScope)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsDSDelete(std::string, SAFplus::EventChannelScope channelScope)
    {
            return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointAll(void)
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptUserInfoCheckPoint(void)
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptECHInfoCheckPoint(void)
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsInfoCheckPoint(void)
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptReconstruct(void)
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointInitialize()
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointFinalize()
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointChannelOpen(EventMessageProtocol *message, int length)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        eventCkpt.write(message->channelName,*val);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointChannelClose(std::string channelName)
    {
        m_checkpoint.remove(name);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointSubscribe(EventMessageProtocol message)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        eventCkpt.write(name,*val);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptCheckPointUnsubscribe(EventMessageProtocol message)
    {
        size_t valLen = length;
        char vdata[sizeof(Buffer)-1+valLen];
        Buffer* val = new(vdata) Buffer(valLen);
        memcpy(val->data, message, valLen);
        eventCkpt.write(name,*val);
        return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptUserInfoRead()
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptECHInfoRead()
    {
                return CL_TRUE;
    }

    ClRcT EventCkpt::eventCkptSubsInfoRead()
    {
                return CL_TRUE;
    }


} /* namespace SAFplus */





