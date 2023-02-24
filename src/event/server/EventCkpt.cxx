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
	m_checkpoint.init(SAFplus::EVENT_CKPT, Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024 * 1024, SAFplusI::CkptDefaultRows);
	m_checkpoint.name = "SAFplus_Event";
	return CL_OK;
}

ClRcT EventCkpt::eventCkptExit(void)
{
	return CL_TRUE;
}


ClRcT EventCkpt::eventCkptCheckPointChannelOpen(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	size_t valLen = request->ByteSize();
	char vdata[sizeof(Buffer) - 1 + valLen];
	Buffer* val = new (vdata) Buffer(valLen);
        request->SerializeToArray(val->data, valLen);	
         
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "write event channel open request to checkpoint with key (len:%d) %lx, val (len:%d) %x", (int)sizeof(key), key, val->len(), *((uint32_t*) val->data));
        
	m_checkpoint.write(key, *val);

	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{        
        
        EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "removing key (len:%d) %lx from event checkpoint", (int)sizeof(key), key);
	m_checkpoint.remove(key);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribe(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{	
        size_t valLen = request->ByteSize();
        char vdata[sizeof(Buffer) - 1 + valLen];
	Buffer* val = new (vdata) Buffer(valLen);
        request->SerializeToArray(val->data, valLen);
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_SUBSCRIBER;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "write event channel subscribe request  to checkpoint with key (len:%d) %lx, val (len:%d) %x", (int)sizeof(key), key, val->len(), *((uint32_t*) val->data));
	m_checkpoint.write(key, *val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{	
        size_t valLen = request->ByteSize();
        char vdata[sizeof(Buffer) - 1 + valLen];
	Buffer* val = new (vdata) Buffer(valLen);
        request->SerializeToArray(val->data, valLen);
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_PUBLISHER;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "write event channel publish request  to checkpoint with key (len:%d) %lx, val (len:%d) %x", (int)sizeof(key), key, val->len(), *((uint32_t*) val->data));
	m_checkpoint.write(key, *val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribe(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_SUBSCRIBER;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "removing key (len:%d) %lx from event checkpoint", (int)sizeof(key), key);
	m_checkpoint.remove(key);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient.id[0] = request->clienthandle().id0();
        keyData.evtClient.id[1] = request->clienthandle().id1();
	keyData.type = EventMessageType::EVENT_CHANNEL_PUBLISHER;
	const uintcw_t key = hash_value(keyData);
        logDebug("EVT", "CKPT", "removing key (len:%d) %lx from event checkpoint", (int)sizeof(key), key);
	m_checkpoint.remove(key);
	return CL_TRUE;
}

