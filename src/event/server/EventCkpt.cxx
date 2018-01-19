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
  EventKeyId keyData;
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  keyData.channelName = request->channelname();
  keyData.evtClient.id[0] = request->clienthandle().id0();
  keyData.evtClient.id[1] = request->clienthandle().id1();
  keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
  char vdata[sizeof(Buffer) + eventMessage.length()];
  Buffer* val = new (vdata) Buffer(eventMessage.length());
  memcpy(val->data,eventMessage.c_str(),eventMessage.length());
  eventKey key(hash_value(keyData),eventMessage.length());
  m_checkpoint.write(key.str(), *val);
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  EventKeyId keyData;
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  keyData.channelName = request->channelname();
  keyData.evtClient.id[0] = request->clienthandle().id0();
  keyData.evtClient.id[1] = request->clienthandle().id1();
  keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
  eventKey key(hash_value(keyData),eventMessage.length());
  m_checkpoint.remove(key.str());
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribeOrPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  EventKeyId keyData;
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  char vdata[sizeof(Buffer)+ eventMessage.length()];
  Buffer* val = new (vdata) Buffer(request->ByteSize());
  memcpy(val->data, eventMessage.c_str(), eventMessage.length());
  keyData.channelName = request->channelname();
  keyData.evtClient.id[0] = request->clienthandle().id0();
  keyData.evtClient.id[1] = request->clienthandle().id1();
  keyData.type = EventMessageType(request->type());
  eventKey key(hash_value(keyData),eventMessage.length());
  m_checkpoint.write(key.str(), *val);
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribeOrUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  EventKeyId keyData;
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  keyData.channelName = request->channelname();
  keyData.evtClient.id[0] = request->clienthandle().id0();
  keyData.evtClient.id[1] = request->clienthandle().id1();
  if(EventMessageType::EVENT_CHANNEL_UNPUBLISHER == EventMessageType(request->type()))
  {
     keyData.type = EventMessageType::EVENT_CHANNEL_PUBLISHER;
  }else{
     keyData.type = EventMessageType::EVENT_CHANNEL_SUBSCRIBER;
  }
  eventKey key(hash_value(keyData),eventMessage.length());
  m_checkpoint.remove(key.str());
  return CL_TRUE;
}

