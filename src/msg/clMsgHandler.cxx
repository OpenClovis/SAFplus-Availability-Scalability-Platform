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

#include "clMsgHandler.hxx"

namespace SAFplus
{

    MsgHandler::MsgHandler()
    {
        // TODO Auto-generated constructor stub

    }

    MsgHandler::~MsgHandler()
    {
        // TODO Auto-generated destructor stub
    }

    void MsgHandler::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
    {
      // 
    Message* msg = msgHead;
    while(msg)
      {
        assert(msg->firstFragment == msg->lastFragment);  // TODO: This code is only written to handle one fragment.
        MsgFragment* frag = msg->firstFragment;
        msgHandler(msg->getAddress(),svr,(ClPtrT)frag->read(0),frag->len,cookie);
        msg = msg->nextMsg;
      }
    msgHead->msgPool->free(msgHead);
    }

    void
    MsgHandler::msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      clDbgCodeError(0,"Application should have implemented this virtual function"); 
    }

} /* namespace SAFplus */
