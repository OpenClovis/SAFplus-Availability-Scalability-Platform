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
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "clMgtMsgHandler.hxx"
#include "clMsgServer.hxx"
#include "../client/MgtMsg.pb.h"
#include <clLogApi.hxx>

using namespace std;

namespace SAFplus
  {

    MgtMsgHandler::MgtMsgHandler()
      {
        // TODO Auto-generated constructor stub

      }

    MgtMsgHandler::~MgtMsgHandler()
      {
        // TODO Auto-generated destructor stub
      }

    void MgtMsgHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
      {
        Mgt::Msg::MsgMgt mgtMsgReq;
        mgtMsgReq.ParseFromArray(msg, msglen);
        if (mgtMsgReq.type() == Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND)
          {
            Mgt::Msg::MsgBind bindData;
            bindData.ParseFromString(mgtMsgReq.bind());
            logDebug("MGT", "BIND", "Handle: 0x%lx.0x%lx, , Module: %s, Route: %s, ", bindData.handle().id0(), bindData.handle().id1(), bindData.module().c_str(),
                bindData.route().c_str());
          }
      }
  } /* namespace SAFplus */
