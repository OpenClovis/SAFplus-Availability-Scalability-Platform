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
#pragma once

#ifndef CLSAFPLUSMSGSERVER_HXX_
#define CLSAFPLUSMSGSERVER_HXX_

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/condition_variable.hpp>
#include <clThreadApi.hxx>
#include <clMsgServer.hxx>

//? <section name="Messaging">

namespace SAFplus
{
  class Wakeable;

#define MSGSIZE 6096
    struct MsgReply
    {
        char buffer[MSGSIZE];
        ClWordT len;
    };

  //? <class> This class wraps the message transport layer, providing the mechanism to multiplex and demultiplex messages of different types onto a single underlying communications port.
  // This class is used for internal SAFplus communications and so there can be only one instance of this class per application (see <ref type="variable">safplusMsgServer</ref>).  This class is publicly accessible because applications are welcome to responsibly multiplex their communications into the same port and server as SAFplus communcations.  See <ref type="file">clMsgPortsAndTypes.hxx</ref> for a list of reserved message types.
    class SafplusMsgServer : public SAFplus::MsgServer
    {
        public:
      // Do not construct this class yourself
            SafplusMsgServer() {};
            SafplusMsgServer(ClWordT port, ClWordT maxPendingMsgs = 0, ClWordT maxHandlerThreads = 1, Options flags = DEFAULT_OPTIONS) { init(port, maxPendingMsgs, maxHandlerThreads, flags); }
            void init(ClWordT port, ClWordT maxPendingMsgs = 0, ClWordT maxHandlerThreads = 1, Options flags = DEFAULT_OPTIONS);
            ~SafplusMsgServer() {
                msgSendConditionMutex.notify_all();
            };

    /*? Provide a callback object that handles a particular type of message
        <arg name='type'>A number from 0 to 255 indicating the message type.  A particular message type identifies a particular handler object.  Make sure that you use unique message types!</arg>
        <arg name='handler'>Your handler function</arg>
        <arg name='cookie'>This pointer will be passed back to your derived MsgHandler::msgHandler() handler function.  You can use it to establish context.</arg>
     */
            void registerHandler(ClWordT type, MsgHandler *handler, ClPtrT cookie);

    /*? Remove the handler for particular type of message 
        <arg name='type'>The type that you passed when RegisterHandler() was called</arg>
     */
            void removeHandler(ClWordT type);

    /*? Send a message with notification or block until a reply is received (see <ref type="class">Wakeable</ref>)
        <arg name='destination'> Address of the destination node/process</arg>
        <arg name='buffer'> Your data</arg>
        <arg name='length'> Your data length</arg>
        <arg name='msgtype'> The destination message handler's type (that it passed when calling RegisterHandler())</arg>
        <arg name='wakeable'> The object describing the response handling semantics (callback, block, etc)</arg>

        Raises the "Error" Exception if something goes wrong.
    */
            MsgReply* sendReply(Handle destination, void* buffer, ClWordT length,ClWordT msgtype=0, Wakeable *wakeable = NULL);

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
    }; //? </class>

  //? The singleton safplusMsgServer object.  Exactly one of these objects exists in every SAFplus-enabled application.  SAFplus uses this object for internal communiations.  You can use it as well by registering your own mesage handlers (objects that implement the SAFplus::MsgHandler interface) via the registerHandler() function.  SAFplus reserved message types are defined in <ref type="file">clMsgPortsAndTypes.hxx</ref>
extern SAFplus::SafplusMsgServer safplusMsgServer;  // Application needs to initialize this with the proper port, etc before use
}


//? </section>
#endif /* CLSAFPLUSMSGSERVER_HXX_ */
