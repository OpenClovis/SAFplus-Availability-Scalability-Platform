/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
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

#include "clMgtNotify.hxx"

#include <clSafplusMsgServer.hxx>
#include <clIocPortList.hxx>
#include "clMgtMsg.hxx"
#include "clMgtRoot.hxx"
#include "MgtMsg.pb.hxx"

extern "C"
{
#include <clCommonErrors.h>
} /* end extern 'C' */

#include <clLogApi.hxx>

using namespace std;

namespace SAFplus
{
  MgtNotify::MgtNotify(const char* nam):MgtObject(nam)
  {
    Module.assign("");
  }

  MgtNotify::~MgtNotify()
  {
  }

  void MgtNotify::addLeaf(std::string leaf, std::string defaultValue)
  {
    mLeafList.insert(pair<string, string>(leaf, defaultValue));
  }
  void MgtNotify::setLeaf(std::string leaf, std::string value)
  {
    mLeafList[leaf] = value;
  }

  void MgtNotify::getLeaf(std::string leaf, std::string *value)
  {
    *value = mLeafList[leaf];
  }

  void MgtNotify::sendNotification(SAFplus::Handle myHandle,std::string route)
  {
    if (!strcmp(Module.c_str(), ""))
    {
      logError("MGT", "NOTI", "Cannot send Notification [%s]", tag.c_str());
      return;
    }
    string bindStr, notiStr, msgRequestStr;
    Mgt::Msg::MsgBind   bindData;
    Mgt::Msg::MsgSetGet notiData;
    Mgt::Msg::MsgMgt    msgRequest;
    string data;
    bindData.set_module(this->Module);
    bindData.set_route(route);
    Mgt::Msg::Handle *hdl = bindData.mutable_handle();
    hdl->set_id0(myHandle.id[0]);
    hdl->set_id1(myHandle.id[1]);

    char strTemp[CL_MAX_NAME_LENGTH];
    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", this->tag.c_str());
    data.append(strTemp);

    map<std::string, std::string>::iterator mapIndex;
    for (mapIndex = mLeafList.begin(); mapIndex != mLeafList.end(); ++mapIndex)
    {
      std::string leafName = (*mapIndex).first;
      std::string leafVal = mLeafList[leafName];

      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>",
               leafName.c_str());
      data.append(strTemp);
      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "%s", leafVal.c_str());
      data.append(strTemp);
      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>",
               leafName.c_str());
      data.append(strTemp);
    }

    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", this->tag.c_str());
    data.append(strTemp);

    /* Pack the request message */
    bindData.SerializeToString(&bindStr);
    notiData.set_data(data);
    notiData.SerializeToString(&notiStr);
    msgRequest.set_bind(bindStr);
    msgRequest.add_data(notiStr);
    msgRequest.set_type(Mgt::Msg::MsgMgt::CL_MGT_MSG_NOTIF);
    msgRequest.SerializeToString(&msgRequestStr);

    /* Send notification */
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;
    SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
    try
    {
      mgtIocInstance->SendMsg(allNodeReps,(void *) msgRequestStr.c_str(), msgRequestStr.size(), SAFplusI::CL_MGT_MSG_TYPE);
    }
    catch(SAFplus::Error &ex)
    {
      logDebug("MGT","NOTI","Send notification failed!");
    }
  }
}
