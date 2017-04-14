/*
 * EventChannel.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTCHANNEL_HXX_
#define EVENTCHANNEL_HXX_
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;

namespace SAFplus {

class SubscriberCkpt
{
public:
    boost::intrusive::list_member_hook<> m_memberHook;
};

class EventChannelUser
{
public:
    SAFplus::Handle evtHandle;   /* The handle of client */
    boost::intrusive::list_member_hook<> m_memberHook;
};


typedef list<SubscriberCkpt, member_hook<SubscriberCkpt, list_member_hook<>, &SubscriberCkpt::m_memberHook>> SubscriberCkptList;
typedef list<EventChannelUser, member_hook<EventChannelUser, list_member_hook<>, &EventChannelUser::m_memberHook>> EventChannelUserList;


typedef struct EvtChannelDB
{
	SAFplus::Mutex channelLevelMutex;   /* Mutex will protect channelId level */
	EventChannelUserList evtChannelUserInfo;    /* List of users who are opened event channel */
    SubscriberCkptList subsCkptMsg;  /* To hold the msg buffer for Check Pointing Subscriber Info */
    int subscriberRefCount;   /* No of users opened this channel for subscription purpose */
    int publisherRefCount;    /* No of users opened this channel for publishing purpose */
    int channelScope;  /* channel scope */
} EventChannelDB;

class EventChannel
{
public:
    boost::intrusive::list_member_hook<> m_memberHook;
    SAFplus::Handle evtChannelHandle;   /* The handle created in clEventInitialize */
    ClNameT evtChannelName;
    EvtChannelDB channelDb;
	EventChannel();
	virtual ~EventChannel();
	void initChannelDB();
	void addChannelSubs();
	void deleteChannelSubs();

};

typedef list<EventChannel, member_hook<EventChannel, list_member_hook<>, &EventChannel::m_memberHook>> EventChannelList;

} /* namespace SAFplus */
#endif /* EVENTCHANNEL_HXX_ */
