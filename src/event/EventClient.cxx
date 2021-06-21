/*
 * EventClient.cxx
 *
 *  Created on: Apr 13, 2017
 *      Author: minhgiang
 */

#include "EventClient.hxx"
#include "common/EventChannel.hxx"
#include <clLogIpi.hxx>
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
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
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <clNameApi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <clObjectMessager.hxx>
#include <time.h>

namespace SAFplus
{

    EventClient::~EventClient()
    {
        // TODO Auto-generated destructor stub
    }

    void EventClient::eventInitialize(clientHandle evtHandle, ClEventCallbacksT * pEvtCallbacks)
    {
        clientHandle = evtHandle;
        if (!eventMsgServer)
        {
            eventMsgServer = &safplusMsgServer;
        }
    }

    ClRcT EventClient::eventChannelOpen(std::string evtChannelName, EventChannelScope scope, SAFplus::Handle &channelHandle)
    {

        EventMessageProtocol sndMessage;
        memset(&sndMessage,0,sizeof(EventMessageProtocol));
        sndMessage.init(clientHandle,evtChannelName,scope,SAFplus::EventMessageType::EVENT_CHANNEL_CREATE);
        sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),scope);
        return CL_OK;
    }

    ClRcT EventClient::eventChannelClose(std::string evtChannelName)
    {
        EventMessageProtocol sndMessage;
        memset(&sndMessage,0,sizeof(EventMessageProtocol));
        sndMessage.init(clientHandle,evtChannelName,scope,SAFplus::EventMessageType::EVENT_CHANNEL_CLOSE);
        sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),scope);
        return CL_OK;
    }

    ClRcT EventClient::eventChannelUnlink(std::string evtChannelName)
    {
        EventMessageProtocol sndMessage;
        memset(&sndMessage,0,sizeof(EventMessageProtocol));
        sndMessage.init(clientHandle,evtChannelName,scope,SAFplus::EventMessageType::EVENT_CHANNEL_UNLINK);
        sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),scope);
        return CL_OK;
    }

    ClRcT EventClient::eventPublish(const void *pEventData, int eventDataSize, std::string channelName)
    {
        char msgPayload[sizeof(EventMessageProtocol)-1 + eventDataSize];
        memset(&msgPayload,0,sizeof(EventMessageProtocol)-1 + eventDataSize);  // valgrind
        EventMessageProtocol *sndMessage = (EventMessageProtocol *)&msgPayload;
        sndMessage.init(clientHandle,evtChannelName,scope,SAFplus::EventMessageType::EVENT_CHANNEL_PUBLISHER);
        memcpy(sndMessage->data,(const void*) pEventData,eventDataSize);
        sendEventMessage((void *)&sndMessage,sizeof(EventMessageProtocol),scope);
        return CL_OK;

    }

    ClRcT EventClient::eventPublish(const void *pEventData, int eventDataSize, SAFplus::Handle handle)
    {
        return CL_OK;
    }

    void EventClient::sendEventMessage(void* data, int dataLength, SAFplus::EventChannelScope eventScope)
    {
        logDebug(FAULT, FAULT_ENTITY, "Sending Event Message");
        Handle activeServer;
        if (severHandle == INVALID_HDL)
        {
            logError(EVENT, EVENT_ENTITY, "No active server... Return");
            return;
        }
        else
        {
            activeServer = severHandle;
        }
        switch (eventScope)
        {
            case SAFplus::EventChannelScope::EVENT_LOCAL_CHANNEL:
            {
                logDebug(EVENT, EVENT_ENTITY, "Send Event message to local Event Server");
                try
                {
                    Handle hdl = getProcessHandle(SAFplusI::EVENT_IOC_PORT);
                    eventMsgServer->SendMsg(hdl, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
                } catch (...) // SAFplus::Error &e)
                {
                    logDebug(EVENT, EVENT_ENTITY, "Failed to send.");
                }
                break;
            }
            case SAFplus::EventChannelScope::EVENT_GLOBAL_CHANNEL:
            {
                logDebug(FAULT, EVENT_ENTITY, "Send  Event message to active Fault Server  : node Id [%d]", activeServer.getNode());
                try
                {
                    eventMsgServer->SendMsg(activeServer, (void *) data, dataLength, SAFplusI::EVENT_MSG_TYPE);
                } catch (...)
                {
                    logDebug(EVENT, "MSG", "Failed to send.");
                }
                break;
            }
        }
    }
} /* namespace SAFplus */
