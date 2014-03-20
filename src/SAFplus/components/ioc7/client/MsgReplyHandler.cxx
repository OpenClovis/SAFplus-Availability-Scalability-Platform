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

#include <iostream>
#include "MsgReplyHandler.hxx"
#include "clSafplusMsgServer.hxx"
#include "clLogApi.hxx"

namespace SAFplus
{
    class SafplusMsgServer;

    MsgReplyHandler::MsgReplyHandler()
    {
        // TODO Auto-generated constructor stub

    }

    MsgReplyHandler::~MsgReplyHandler()
    {
        // TODO Auto-generated destructor stub
    }

    void
    MsgReplyHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        /**
         * TODO:
         * process ONE or ALL or FOREVER
         */
        if (cookie)
        {
            memcpy(cookie, msg, msglen);
        }

        SAFplus::SafplusMsgServer *safplusMsgServer = reinterpret_cast<SAFplus::SafplusMsgServer*>(svr);

        //Signal to wake
        safplusMsgServer->condMsgSendReplyMutex.notify_all();

    }

} /* namespace SAFplus */
