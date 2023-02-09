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


namespace SAFplus
{

class eventKey
{
public:
	int length;
	int id;
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
  boost::hash_combine(seed,h.evtClient.getNode());
  boost::hash_combine(seed,h.evtClient.getPort());
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
