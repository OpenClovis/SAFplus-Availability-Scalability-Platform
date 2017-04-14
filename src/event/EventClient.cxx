/*
 * EventClient.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventClient.hxx"

namespace SAFplus {



EventClient::~EventClient()
{
	// TODO Auto-generated destructor stub
}

ClRcT eventInitialize(clientHandle evtHandle, ClEventCallbacksT * pEvtCallbacks)
{

}

ClRcT eventChannelOpen(std::string evtChannelName,EventChannelType evtChannelOpenFlag, EventChannelScope scope, SAFplus::Handle &channelHandle);
{
}

ClRcT eventChannelClose(SAFplus::Handle channelHandle)
{

}

ClRcT eventChannelUnlink(std::string evtChannelName)
{

}

ClRcT eventPublish(const void *pEventData,int eventDataSize, std::string channelName)
{

}

ClRcT eventPublish(const void *pEventData,int eventDataSize, SAFplus::Handle handle)
{

}
} /* namespace SAFplus */
