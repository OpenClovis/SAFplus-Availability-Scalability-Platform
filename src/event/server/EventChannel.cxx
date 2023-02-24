/*
 * EventChannel.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventChannel.hxx"

using namespace SAFplusI;
using namespace SAFplus;
#include <clLogApi.hxx>

EventChannel::EventChannel() {
	// TODO Auto-generated constructor stub

}

EventChannel::~EventChannel() {
	// TODO Auto-generated destructor stub
}

void EventChannel::addChannelSub(EventSubscriber& sub)
{
	if(this->eventSubs.empty())
	{
		logDebug("EVT","SUB","Add Subscriber[%d,%d] to subscriber list",sub.usr.evtHandle.getNode(),sub.usr.evtHandle.getPort());
		this->eventSubs.push_back(sub);
	}
	else
	{
		for (EventSubscriberList::iterator iter = eventSubs.begin(); iter != eventSubs.end(); iter++)
		{
			EventSubscriber &s = *iter;
			if(s.usr.evtHandle==sub.usr.evtHandle)
			{
				logDebug("EVT","SUB","Subscriber[%d,%d] is already exist in subscriber list...",sub.usr.evtHandle.getNode(),sub.usr.evtHandle.getPort());
				throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS,"Subscriber is already exist...", __FILE__, __LINE__);
				return;
			}
		}
    	logDebug("EVT","SUB","Add Subscriber[%d,%d] to subscriber list",sub.usr.evtHandle.getNode(),sub.usr.evtHandle.getPort());
		this->eventSubs.push_back(sub);
	}
}

void EventChannel::addChannelPub(EventPublisher& pub)
{
	if(this->eventPubs.empty())
	{
		logDebug("EVT","SUB","Add Publisher[%d,%d] to publisher list",pub.usr.evtHandle.getNode(),pub.usr.evtHandle.getPort());
		this->eventPubs.push_back(pub);
	}
	else
	{
		for (EventPublisherList::iterator iter = eventPubs.begin(); iter != eventPubs.end(); iter++)
		{
			EventPublisher &s = *iter;
			if(s.usr.evtHandle==pub.usr.evtHandle)
			{
				logDebug("EVT","SUB","Publisher[%d,%d] is already exist in publisher list...",pub.usr.evtHandle.getNode(),pub.usr.evtHandle.getPort());
				throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::EXISTS,"Publisher is already exist...", __FILE__, __LINE__);
				return;
			}
		}
		logDebug("EVT","SUB","Add Publisher[%d,%d] to publisher list",pub.usr.evtHandle.getNode(),pub.usr.evtHandle.getPort());
		this->eventPubs.push_back(pub);
	}
}



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

void EventChannel::deleteChannelSub(SAFplus::Handle handle)
{
    //TODO delete all sub with sub handle
    if(eventPubs.empty() && eventSubs.empty())
    {
    	logDebug("EVT", "MSG", "Event subscriber and publisher list is empty");
	//	throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Subscriber doesn't exist", __FILE__, __LINE__);
        return;
    }
    for (EventPublisherList::iterator iter = eventPubs.begin(); iter != eventPubs.end(); iter++)
    {
        EventPublisher &evtPub = *iter;
        if(evtPub.usr.evtHandle==handle)
        {
            logDebug("EVT", "MSG", "Remove publisher[%d:%d] ...",handle.getNode(),handle.getPort());
            eventPubs.erase_and_dispose(iter, eventPub_delete_disposer());
            break;
        }
    }
    for (EventSubscriberList::iterator iter = eventSubs.begin(); iter != eventSubs.end(); iter++)
    {
        EventSubscriber &evtSub = *iter;
        if(evtSub.usr.evtHandle==handle)
        {
            logDebug("EVT", "MSG", "Remove Subscriber[%d:%d] ...",handle.getNode(),handle.getPort());
            eventSubs.erase_and_dispose(iter, eventSub_delete_disposer());
            return;
        }
    }
//	throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Subscriber doesn't exist", __FILE__, __LINE__);
}

void EventChannel::deleteChannelPub(SAFplus::Handle pubHandle)
{
    //TODO delete all sub with sub handle
    if(eventPubs.empty())
    {
        logDebug("EVT", "MSG", "Event publish list is empty");
//		throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Publisher doesn't exist", __FILE__, __LINE__);
        return;
    }
    for (EventPublisherList::iterator iter = eventPubs.begin(); iter != eventPubs.end(); iter++)
    {
        EventPublisher &evtPub = *iter;
        if(evtPub.usr.evtHandle==pubHandle)
        {
            eventPubs.erase_and_dispose(iter, eventPub_delete_disposer());
            return;
        }
    }
    //throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::DOES_NOT_EXIST,"Publisher doesn't exist", __FILE__, __LINE__);

}
