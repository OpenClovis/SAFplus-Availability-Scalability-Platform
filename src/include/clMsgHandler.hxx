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

#ifndef CLMSGHANDLER_HXX_
#define CLMSGHANDLER_HXX_

#include <clCommon.hxx>
#include <clMsgBase.hxx>
#include <clHandleApi.hxx>

namespace SAFplus
  {
  class MsgServer;
  }

#if 0
namespace SAFplusI
  {
  // derive from this class to add message handling functionality to your object
  class MsgHandlerI
    {
  public:
    virtual
    ~MsgHandlerI() {};

  public:
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie) = 0;
    };
  }
#endif

//? <section name="Messaging">

namespace SAFplus
  {

    //? <class> Interface class to describe how to receive messages.  Derive your class from this class and override the msgHandler function so that your class can receive messages via callback.
    class MsgHandler //: public SAFplusI::MsgHandlerI
    {
  public:
    MsgHandler();
    virtual
    ~MsgHandler();
  public:
      //? Override one of these two msgHandler functions to receive messages.  This function provides a simple buffer and value interface but is not as efficient as the function that provides the Message* object because this function may need to copy fragmented messages into a newly allocated contiguous buffer.
      // <arg name="from"> Who sent this message.  This handle will only resolve as deeply as the message transport layer can see.  Typically this means it resolves to the sending node and port/process</arg>
      // <arg name="svr"> The object that received this message.  You can use this object to send a reply or disambiguate received messages if your object is subscribed to receive messages from multiple sources</arg>
      // <arg name="msg"> The message </arg>
      // <arg name="msglen"> The length of the message </arg>
      // <arg name="cookie"> You passed this data to the MsgServer when you registered to receive messages.  It gives the data back. Used for context tracking</arg>
    virtual void msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

      //? Override this function to handle received messages.  If this function is not overridden, its default implementation converts the Message* object into a contiguous buffer and calls the msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie) function.  Therefore it is more efficient for applications to override this function directly. Application programmers can refer to this function's implementation as an example of how to properly interprete the Message* object.
      // <arg name="svr"> The object that received this message.  You can use this object to send a reply or disambiguate received messages if your object is subscribed to receive messages from multiple sources </arg>
      // <arg name="msg"> The message.  Note that this may be a linked list of multiple messages. </arg>
      // <arg name="cookie"> You passed this data to the MsgServer when you registered to receive messages.  This can be used by the application for context tracking</arg>
    virtual void msgHandler(MsgServer* svr, Message* msg, ClPtrT cookie);
    };  //? </class>
  } /* namespace SAFplus */

//? </section>

#endif /* CLMSGHANDLER_HXX_ */
