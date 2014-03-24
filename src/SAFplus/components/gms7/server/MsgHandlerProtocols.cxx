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
#include "MsgHandlerProtocols.hxx"
#include "clMsgServer.hxx"
#include "groupServer.hxx"
using namespace std;

namespace SAFplus
{

void MsgHandlerProtocols::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  messageProtocol *rxMsg = (messageProtocol *)msg;
  /* If local message, ignore */
  if(from.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
  {
    return;
  }
  switch(rxMsg->messageType)
  {
    case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
      nodeJoinHandle();
      break;
    case CL_IOC_NODE_LEAVE_NOTIFICATION:
      nodeLeaveHandle();
      break;
    case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
      compJoinHandle();
      break;
    case CL_IOC_COMP_DEATH_NOTIFICATION:
      compLeaveHandle();
      break;
    default: //Unsupported
      break;
  }
}

} /* namespace SAFplus */
