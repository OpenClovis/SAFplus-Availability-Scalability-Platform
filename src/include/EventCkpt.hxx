/*
 * EventCkpt.hxx
 *
 *  Created on: May 3, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCKPT_HXX_
#define EVENTCKPT_HXX_
#include "EventChannel.hxx"
#include "EventCommon.hxx"
#include "clCkptApi.hxx"
#include <string>
#include <clCommon.hxx>
#include <rpcEvent.hxx>
#include <sstream>

namespace SAFplus
{

class eventKey
{
public:
  std::size_t id;
  int length;

  eventKey()
  {
     id = 0;
     length = 0;
  }
  eventKey(const size_t& iid,const int& ilength)
  {
     id = iid;
     length = ilength;
  }
  eventKey(const std::string& str)
  {
     std::istringstream iss(str);
     iss >> id;
     iss >> length;
  }
  std::string str()
  {
     std::ostringstream oss;
     oss << id <<" "<< length;
     return oss.str();
  }
};

class EventKeyId
{
public:
	std::string channelName;
	Handle evtClient;
	EventMessageType type;
};

inline std::size_t hash_value(EventKeyId const& h)
{

  std::size_t seed = 0;
  boost::hash_combine(seed,h.channelName);
  boost::hash_combine(seed,h.evtClient.id[0]);
  boost::hash_combine(seed,h.evtClient.id[1]);
  boost::hash_combine(seed,h.type);
  return seed;
}

class EventCkpt
{
public:
	SAFplus::Checkpoint m_checkpoint;

	EventCkpt();
	virtual ~EventCkpt();
	ClRcT eventCkptInit(void);
	ClRcT eventCkptExit(void);
	ClRcT eventCkptSubsDSCreate(char* *pChannelName,  EventChannelScope channelScope);
	ClRcT eventCkptSubsDSDelete(char*, EventChannelScope channelScope);
	ClRcT eventCkptCheckPointChannelOpen(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request);
	ClRcT eventCkptCheckPointChannelClose(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request);
	ClRcT eventCkptCheckPointSubscribeOrPublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request);
	ClRcT eventCkptCheckPointUnsubscribeOrUnpublish(const SAFplus::Rpc::rpcEvent::eventChannelRequest* request);

};

} /* namespace SAFplus */
#endif /* EVENTCKPT_HXX_ */
