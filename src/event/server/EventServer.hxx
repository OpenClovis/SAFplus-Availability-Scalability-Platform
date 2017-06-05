/*
 * EventServer.hxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#ifndef EVENTSERVER_HXX_
#define EVENTSERVER_HXX_


#include <../common/EventChannel.hxx>
#include <../common/EventCommon.hxx>
#include <EventCkpt.hxx>
#include <clGroupApi.hxx>
#include <string>
#include "clMsgHandler.hxx"
#include "clMsgServer.hxx"
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <FaultSharedMem.hxx>
#include <clHandleApi.hxx>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <clCustomization.hxx>
#include <clNameApi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <time.h>



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
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    void eventChannelCreateHandle(EventMessageProtocol *rxMsg);
    void eventChannelCloseHandle(EventMessageProtocol *rxMsg);
    void eventChannelSubsHandle(EventMessageProtocol *rxMsg);
    void eventChannelUnSubsHandle(EventMessageProtocol *rxMsg);
    void eventpublish(EventMessageProtocol *rxMsg,ClWordT msglen);
    bool eventloadGlobalchannel();
    bool eventloadLocalchannel();

};

} /* namespace SAFplus */
#endif /* EVENTSERVER_HXX_ */
