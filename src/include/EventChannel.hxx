/*
 * EventChannel.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCHANNEL_HXX_
#define EVENTCHANNEL_HXX_
#include <boost/intrusive/list.hpp>
#include <string>
#include <clCommon.hxx>
#include <clThreadApi.hxx>
#include "EventCommon.hxx"





//using namespace boost::intrusive;

namespace SAFplus {

class EventChannelUser
{
public:
	std::string channelName;
	SAFplus::Handle evtHandle;   /* The handle of client */
};


class EventSubscriber
{
public:
	boost::intrusive::list_member_hook<> m_memberHook;
	EventChannelUser usr;
	EventSubscriber(SAFplus::Handle evtClient, std::string channelName)
	{
		usr.evtHandle=evtClient;
		usr.channelName=channelName;
	}
	virtual ~EventSubscriber()
	{

	}


};

class EventPublisher
{
public:
	boost::intrusive::list_member_hook<> m_memberHook;
	EventChannelUser usr;
	EventPublisher(SAFplus::Handle evtClient, std::string channelName)
	{
		usr.evtHandle=evtClient;
		usr.channelName=channelName;
	}
	virtual ~EventPublisher()
	{

	}
};


typedef boost::intrusive::list<EventSubscriber, boost::intrusive::member_hook<EventSubscriber, boost::intrusive::list_member_hook<>, &EventSubscriber::m_memberHook>> EventSubscriberList;
typedef boost::intrusive::list<EventPublisher, boost::intrusive::member_hook<EventPublisher, boost::intrusive::list_member_hook<>, &EventPublisher::m_memberHook>> EventPublisherList;


class EventChannel
{
public:
	boost::intrusive::list_member_hook<> m_memberHook;
	SAFplus::Handle evtChannelHandle;   /* The handle created in clEventInitialize */
	std::string channelName;
	EventChannelScope scope;
	SAFplus::Mutex channelLevelMutex;   /* Mutex will protect channelId level */
	EventSubscriberList eventSubs;  /* To hold the msg buffer for Check Pointing Subscriber Info */
	EventPublisherList eventPubs;
	int subscriberRefCount;   /* No of users opened this channel for subscription purpose */
	int publisherRefCount;    /* No of users opened this channel for publishing purpose */

	EventChannel();
	virtual ~EventChannel();
	void addChannelSub(EventSubscriber& sub);
	void addChannelPub(EventPublisher& pub);
	void deleteChannelSub(SAFplus::Handle subHandle);
	void deleteChannelPub(SAFplus::Handle pubHandle);

};

typedef boost::intrusive::list<EventChannel, boost::intrusive::member_hook<EventChannel, boost::intrusive::list_member_hook<>, &EventChannel::m_memberHook>> EventChannelList;

} /* namespace SAFplus */
#endif /* EVENTCHANNEL_HXX_ */
