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
  char handleKey[sizeof(Buffer)+sizeof(eventKey)];
  Buffer* key = new(handleKey) Buffer(sizeof(eventKey));
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  EventKeyId keyData;
  keyData.channelName = request->channelname();
  keyData.evtClient = INVALID_HDL;
  keyData.type = EventMessageType::EVENT_CHANNEL_CREATE;
  char vdata[sizeof(Buffer) + eventMessage.length()];
  Buffer* val = new (vdata) Buffer(eventMessage.length());
  memcpy(val->data,eventMessage.c_str(),eventMessage.length());
  logDebug("EVT", "CKPT", "write eventCkptCheckPointChannel request  to checkpoint with key %ld", key);
  ((eventKey*)key->data)->id= static_cast<uintcw_t>(hash_value(keyData));
  ((eventKey*)key->data)->length= eventMessage.length();
  m_checkpoint.write(*key, *val);
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  char handleKey[sizeof(Buffer)+sizeof(eventKey)];
  Buffer* key = new(handleKey) Buffer(sizeof(eventKey));
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  EventKeyId keyData;
  keyData.channelName = request->channelname();
  keyData.evtClient = INVALID_HDL;
  keyData.type = EventMessageType(request->type());
  logDebug("EVT", "CKPT", "remove eventCkptCheckPointChannel request  to checkpoint with key %d,  length %d", request->ByteSize(), key);
  ((eventKey*)key->data)->id= static_cast<uintcw_t>(hash_value(keyData));
  ((eventKey*)key->data)->length= eventMessage.length();
  m_checkpoint.remove(*key);
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointSubscribeOrPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  char handleKey[sizeof(Buffer) + sizeof(eventKey)];
  Buffer* key = new(handleKey) Buffer(sizeof(eventKey));
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  char vdata[sizeof(Buffer)+ eventMessage.length()];
  Buffer* val = new (vdata) Buffer(request->ByteSize());
  memcpy(val->data, eventMessage.c_str(), eventMessage.length());
  EventKeyId keyData;
  keyData.channelName = request->channelname();
  keyData.evtClient = INVALID_HDL;
  keyData.type = EventMessageType(request->type());
  ((eventKey*)key->data)->id= static_cast<uintcw_t>(hash_value(keyData));
  ((eventKey*)key->data)->length= eventMessage.length();
  m_checkpoint.write(*key, *val);
  return CL_TRUE;
}

ClRcT EventCkpt::eventCkptCheckPointUnsubscribeOrUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request)
{
  char handleKey[sizeof(Buffer) + sizeof(eventKey)];
  Buffer* key = new(handleKey) Buffer(sizeof(eventKey));
  std::string eventMessage;
  request->SerializeToString(&eventMessage);
  EventKeyId keyData;
  keyData.channelName = request->channelname();
  keyData.evtClient = INVALID_HDL;
  keyData.type = EventMessageType(request->type());
  ((eventKey*)key->data)->id = static_cast<uintcw_t>(hash_value(keyData));
  ((eventKey*)key->data)->length = eventMessage.length();
  m_checkpoint.remove(*key);
  return CL_TRUE;
}

