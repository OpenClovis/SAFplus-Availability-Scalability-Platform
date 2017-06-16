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





using namespace boost::intrusive;

namespace SAFplus {

class EventChannelUser
{
public:
	uint64_t channelId;
	SAFplus::Handle evtHandle;   /* The handle of client */
};


class EventSubscriber
{
public:
	boost::intrusive::list_member_hook<> m_memberHook;
	EventChannelUser usr;
	EventSubscriber(SAFplus::Handle evtClient, uint64_t channelId)
	{
		usr.evtHandle=evtClient;
		usr.channelId=channelId;
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
	EventPublisher(SAFplus::Handle evtClient, uint64_t channelId)
	{
		usr.evtHandle=evtClient;
		usr.channelId=channelId;
	}
	virtual ~EventPublisher()
	{

	}
};


typedef list<EventSubscriber, member_hook<EventSubscriber, list_member_hook<>, &EventSubscriber::m_memberHook>> EventSubscriberList;
typedef list<EventPublisher, member_hook<EventPublisher, list_member_hook<>, &EventPublisher::m_memberHook>> EventPublisherList;


class EventChannel
{
public:
	boost::intrusive::list_member_hook<> m_memberHook;
	SAFplus::Handle evtChannelHandle;   /* The handle created in clEventInitialize */
	uintcw_t evtChannelId;
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

typedef list<EventChannel, member_hook<EventChannel, list_member_hook<>, &EventChannel::m_memberHook>> EventChannelList;

} /* namespace SAFplus */
#endif /* EVENTCHANNEL_HXX_ */
