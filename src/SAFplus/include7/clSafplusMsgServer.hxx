/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.
 */

#ifndef CLSAFPLUSMSGSERVER_HXX_
#define CLSAFPLUSMSGSERVER_HXX_

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/condition_variable.hpp>
#include "clMsgHandler.hxx"
#include "clMsgServer.hxx"
#include "clThreadApi.hxx"

namespace SAFplus
{
#define MSGSIZE 4096
    struct MsgReply
    {
        char buffer[MSGSIZE];
    };

    class SafplusMsgServer : public SAFplus::MsgServer
    {
        public:
            SafplusMsgServer() {};
            SafplusMsgServer(ClWordT port, ClWordT maxPendingMsgs = 0, ClWordT maxHandlerThreads = 1, Options flags = DEFAULT_OPTIONS);
            ~SafplusMsgServer() {
                msgSendConditionMutex.notify_all();
            };

            /** Handle a particular type of message
                 @param type    A number from 0 to 255 indicating the message type
                 @param handler Your handler function
                 @param cookie  This pointer will be passed to you handler function
             */
            void
            RegisterHandler(ClWordT type, MsgHandler *handler, ClPtrT cookie);

            /** Remove the handler for particular type of message */
            void
            RemoveHandler(ClWordT type);

            /** Send a message
                 @param msgtype The destination message handler
                 @param destination Address of the destination node/process
                 @param buffer Your data
                 @param length Your data length

             Raises the "Error" Exception if something goes wrong, or if the destination queue does not
             exist.
             */
            MsgReply *SendReply(ClIocAddressT destination, void* buffer, ClWordT length,ClWordT msgtype=0);

        protected:
            MsgHandler *handlers[NUM_MSG_TYPES];
            ClPtrT cookies[NUM_MSG_TYPES];

        public:
            /**
             * msg buffer for reply data
             */
            MsgReply msgReply;

            // Msg sending and wakable on reply
            Mutex      msgSendReplyMutex;
            ThreadCondition msgSendConditionMutex;
    };
}

extern SAFplus::SafplusMsgServer safplusMsgServer;

#endif /* CLSAFPLUSMSGSERVER_HXX_ */
