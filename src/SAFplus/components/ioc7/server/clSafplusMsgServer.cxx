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

#include "clSafplusMsgServer.hxx"

namespace SAFplus
{

    SAFplus::SafplusMsgServer::SafplusMsgServer(ClWordT port, ClWordT maxPendingMsgs, ClWordT maxHandlerThreads, Options flags) :
                    MsgServer(port, maxPendingMsgs, maxHandlerThreads, flags)
    {

    }

    void
    SAFplus::SafplusMsgServer::RegisterHandler(ClWordT type, MsgHandler handler, ClPtrT cookie)
    {
        SAFplus::MsgServer::RegisterHandler(type, handler, cookie);
    }

    void
    SAFplus::SafplusMsgServer::RemoveHandler(ClWordT type)
    {
        SAFplus::MsgServer::RemoveHandler(type);
    }

}
