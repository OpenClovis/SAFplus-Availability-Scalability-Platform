/*
 * EventCkpt::eventCkpt.cxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#include "EventCkpt.hxx"

using namespace SAFplusI;
using namespace SAFplus;

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
	m_checkpoint.init(SAFplus::EVENT_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);
	m_checkpoint.name = "SAFplus_Event";
	return CL_OK ;
}

ClRcT EventCkpt::eventCkptExit(void)
{
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelOpen(EventMessageProtocol* message, int length)
{
	char vdata[sizeof(Buffer)-1+length];
	Buffer* val = new(vdata) Buffer(length);
	memcpy(val->data, message, length);
	EventKey keyData;
	keyData.channelId=message->eventChannelId;
	keyData.evtClient=INVALID_HDL;
	keyData.type=message->messageType;
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.write(key,*val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(EventMessageProtocol* message , int length)
{
	EventKey keyData;
	keyData.channelId=message->eventChannelId;
	keyData.evtClient=INVALID_HDL;
	keyData.type=message->messageType;
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.remove(key);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribeOrPublish(EventMessageProtocol* message , int length)
{
	char vdata[sizeof(Buffer)-1+length];
	Buffer* val = new(vdata) Buffer(length);
	memcpy(val->data, message, length);

	EventKey keyData;
	keyData.channelId=message->eventChannelId;
	keyData.evtClient=INVALID_HDL;
	keyData.type=message->messageType;
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.write(key,*val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribeOrUnpublish(EventMessageProtocol* message , int length)
{
	EventKey keyData;
	keyData.channelId=message->eventChannelId;
	keyData.evtClient=INVALID_HDL;
	keyData.type=message->messageType;
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.remove(key);
	return CL_TRUE;
}









