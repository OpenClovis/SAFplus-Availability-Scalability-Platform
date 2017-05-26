/*
 * EventChannel.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventChannel.hxx"

namespace SAFplus {

EventChannel::EventChannel() {
	// TODO Auto-generated constructor stub

}

EventChannel::~EventChannel() {
	// TODO Auto-generated destructor stub
}

void EventChannel::addChannelSub(EventSubscriber sub)
{
    this->eventSubs.push_back(sub);
}

void EventChannel::addChannelPub(EventPublisher pub)
{
    this->eventPubs.push_back(pub);
}

struct eventChannel_delete_disposer
{
  void operator()(EventChannel *delete_this)
  {
    delete delete_this;
  }
};

struct eventSub_delete_disposer
{
  void operator()(EventSubscriber *delete_this)
  {
    delete delete_this;
  }
};

struct eventPub_delete_disposer
{
  void operator()(EventPublisher *delete_this)
  {
    delete delete_this;
  }
};

void EventChannel::deleteChannelSub(SAFplus::Handle subHandle)
{
    //TODO delete all sub with sub handle
    if(eventSubs.empty())
    {
        return;
    }
    for (EventSubscriberList::iterator iter = eventSubs.begin(); iter != eventSubs.end(); iter++)
    {
        EventSubscriber &evtSub = *iter;
        if(evtSub.usr.evtHandle==subHandle)
        {
            eventSubs.erase_and_dispose(it, eventSub_delete_disposer());
        }
    }

}

void EventChannel::deleteChannelPub(SAFplus::Handle pubHandle)
{
    //TODO delete all sub with sub handle
    if(eventPubs.empty())
    {
        return;
    }
    for (EventPublisherList::iterator iter = eventPubs.begin(); iter != eventPubs.end(); iter++)
    {
        EventPublisher &evtPub = *iter;
        if(evtPub.usr.evtHandle==pubHandle)
        {
            eventPubs.erase_and_dispose(it, eventPub_delete_disposer());
        }
    }

}

} /* namespace SAFplus */
