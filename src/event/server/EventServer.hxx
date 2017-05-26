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

class EventServer:public SAFplus::MsgHandler,public SAFplus::Wakeable
{

public:
	SAFplus::Handle                   severHandle;             // handle for identify a event client
	SAFplus::SafplusMsgServer*        eventMsgServer;       // safplus message for send event message to event server
	SAFplus::Wakeable*                wakeable;             // Wakeable object for change notification
	SAFplus::Handle activeServer;
	EventChannelList localChannelList; //? contain local channel
	EventChannelList globalChannelList; //? contain global channel
    SAFplus::Mutex localChannelListLock;
    SAFplus::Mutex globalChannelListLock;
    int numberOfGlobalChannel;
    int numberOfLocalChannel;
    //? fault server group
    SAFplus::Group group; //? event sever group
    EventCkpt evtCkpt;

	EventServer();
	virtual ~EventServer();
	void initialize();
    void wake(int amt,void* cookie=NULL);
    EventChannel  getChannelbyName(std::string channelName, EventChannelScope &scope);
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    bool eventloadGlobalchannel();
    bool eventloadLocalchannel();

};

} /* namespace SAFplus */
#endif /* EVENTSERVER_HXX_ */
