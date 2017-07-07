/*
 * EventCkpt::eventCkpt.cxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#include "EventCkpt.hxx"

using namespace SAFplusI;
using namespace SAFplus;
using namespace Rpc;
using namespace rpcEvent;

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

ClRcT EventCkpt::eventCkptCheckPointChannelOpen(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	char vdata[sizeof(Buffer)-1+request->ByteSize()];
	Buffer* val = new(vdata) Buffer(request->ByteSize());
	memcpy(val->data, request, request->ByteSize());
	EventKey keyData;
	keyData.channelName=request->channelname();
	keyData.evtClient=INVALID_HDL;
	keyData.type=EventMessageType::EVENT_CHANNEL_CREATE;
	const uintcw_t key = static_cast<uintcw_t>(hash_value(keyData));
	m_checkpoint.write(key,*val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKey keyData;
	keyData.channelName=request->channelname();
	keyData.evtClient=INVALID_HDL;
	keyData.type=EventMessageType(request->type());
	const uintcw_t key = static_cast<uintcw_t>(hash_value(keyData));
	m_checkpoint.remove(key);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribeOrPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	char vdata[sizeof(Buffer)-1+request->ByteSize()];
	Buffer* val = new(vdata) Buffer(request->ByteSize());
	memcpy(val->data, request, request->ByteSize());
	EventKey keyData;
	keyData.channelName=request->channelname();
	keyData.evtClient=INVALID_HDL;
	keyData.type=EventMessageType(request->type());
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.write(key,*val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribeOrUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKey keyData;
	keyData.channelName=request->channelname();
	keyData.evtClient=INVALID_HDL;
	keyData.type=EventMessageType(request->type());
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.remove(key);
	return CL_TRUE;
}









