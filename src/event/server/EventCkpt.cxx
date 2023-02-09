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
	char handleData[sizeof(Buffer)-1+sizeof(int) + sizeof(int)];
	Buffer* key = new(handleData) Buffer(sizeof(eventKey));
	int size = request->ByteSize();
	void *buffer = malloc(size);
	request->SerializeToArray(buffer, size);

	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient = INVALID_HDL;
	keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
	const int keyId = static_cast<uintcw_t>(hash_value(keyData) % 65000);
	size_t valLen = request->ByteSize();
	char vdata[sizeof(Buffer) - 1 + valLen];
	Buffer* val = new (vdata) Buffer(valLen);
	memcpy(val->data,buffer,valLen);
	logDebug("EVT", "CKPT", "write eventCkptCheckPointChannel request  to checkpoint with key (len:%d) %x", key->objectSize(), *((uint32_t*) key->data));
    ((eventKey*)key->data)->id= keyId;
    ((eventKey*)key->data)->length= valLen;
	m_checkpoint.write(*key, *val);

//	SAFplus::Rpc::rpcEvent::eventChannelRequest requesttest;
//	requesttest.ParseFromArray(buffer,valLen);
//	logDebug("NAME", "GET", "check 1 Get object: Name [%s]", requesttest.channelname().c_str());
//	logDebug("NAME", "GET", "check 1 Get object: Scope [%d]", requesttest.scope());
//	logDebug("NAME", "GET", "check 1 Get object: Type [%d]", requesttest.type());
//
//	Checkpoint::Iterator ibegin = m_checkpoint.begin();
//	Checkpoint::Iterator iend = m_checkpoint.end();
//	for (Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
//	{
//
//        SAFplus::Checkpoint::KeyValuePair& item = *iter;
//        int keytt = ((eventKey*) (*item.first).data)->id;
//        int lenghtt = ((eventKey*) (*item.first).data)->length;
//        logDebug("NAME", "GET", "check Get object for key [%ld]", keytt);
//        logDebug("NAME", "GET", "check Data length [%d]", lenghtt);
//
//		BufferPtr& curval = iter->second;
//		if (curval)
//		{
//			SAFplus::Rpc::rpcEvent::eventChannelRequest request;
//			request.ParseFromArray(curval->data,lenghtt);
//			logDebug("NAME", "GET", "check Get object: Name [%s]", request.channelname().c_str());
//			logDebug("NAME", "GET", "check Get object: Scope [%d]", request.scope());
//			logDebug("NAME", "GET", "check Get object: Type [%d]", request.type());
//		}
//	}
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient = INVALID_HDL;
	keyData.type = EventMessageType(request->type());
	const uintcw_t key = static_cast<uintcw_t>(hash_value(keyData));
	logDebug("EVT", "CKPT", "remove eventCkptCheckPointChannel request  to checkpoint with key [%lu],  length [%lu]", key, (unsigned long)request->ByteSize());
	m_checkpoint.remove(key);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribeOrPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	std::string eventMessage;
	request->SerializeToString(&eventMessage);
	char vdata[sizeof(Buffer) - 1 + eventMessage.length() + 1];
	Buffer* val = new (vdata) Buffer(request->ByteSize());
	memcpy(val->data, eventMessage.c_str(), eventMessage.length() + 1);
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient = INVALID_HDL;
	keyData.type = EventMessageType(request->type());
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.write(key, *val);
	return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribeOrUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
	EventKeyId keyData;
	keyData.channelName = request->channelname();
	keyData.evtClient = INVALID_HDL;
	keyData.type = EventMessageType(request->type());
	const uintcw_t key = hash_value(keyData);
	m_checkpoint.remove(key);
	return CL_TRUE;
}

