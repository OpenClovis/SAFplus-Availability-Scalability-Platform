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

namespace SAFplus
{


class EventKey
{
public:
	uintcw_t channelId;
	Handle evtClient;
	EventMessageType type;
};

inline std::size_t hash_value(EventKey const& h)
{

  std::size_t seed = 0;
  boost::hash_combine(seed,h.channelId);
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
	ClRcT eventCkptCheckPointChannelOpen(EventMessageProtocol* message, int length);
	ClRcT eventCkptCheckPointChannelClose(EventMessageProtocol* message, int length);
	ClRcT eventCkptCheckPointSubscribeOrPublish(EventMessageProtocol* message , int length);
	ClRcT eventCkptCheckPointUnsubscribeOrUnpublish(EventMessageProtocol* message , int length);

};

} /* namespace SAFplus */
#endif /* EVENTCKPT_HXX_ */
