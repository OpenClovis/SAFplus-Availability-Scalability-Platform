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

#ifndef MSGREPLYHANDLER_HXX_
#define MSGREPLYHANDLER_HXX_

#include "clMsgHandler.hxx"

namespace SAFplus
{

    /*
     *
     */
    class MsgReplyHandler : public SAFplus::MsgHandler
    {
        public:
            MsgReplyHandler();
            virtual
            ~MsgReplyHandler();

            virtual void
            msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    };

} /* namespace SAFplus */
#endif /* MSGREPLYHANDLER_HXX_ */
