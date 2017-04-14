/*
 * EventServer.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTSERVER_HXX_
#define EVENTSERVER_HXX_


#include <EventChannel.hxx>

namespace SAFplus
{

class EventServer
{

public:
	SAFplus::Handle                   severHandle;             // handle for identify a event client
	SAFplus::SafplusMsgServer*        eventMsgServer;       // safplus message for send event message to event server
	SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification

	EventChannelList localChannelList;
	EventChannelList globalChannelList;

	EventServer();
	virtual ~EventServer();
	void initialize();
};

} /* namespace SAFplus */
#endif /* EVENTSERVER_HXX_ */
