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

#ifdef MGT_ACCESS
#include <clSafplusMsgServer.hxx>
#include <clIocPortList.hxx>
#include "clMgtMsg.hxx"
#include "clMgtRoot.hxx"
#endif

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
    name.assign(nam);
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

  void MgtNotify::sendNotification()
  {
    if (!strcmp(Module.c_str(), ""))
    {
      logError("MGT", "RPC", "Cannot send Notification [%s]", name.c_str());
      return;
    }
#ifdef MGT_ACCESS
    char *buffer = (char *) malloc(MGT_MAX_DATA_LEN);
    if (!buffer)
    {
      return;
    }

//    ClMgtMessageNotifyTypeT *notifyData = (ClMgtMessageNotifyTypeT *)buffer;
//    ClCharT *data = notifyData->data;
//    ClUint32T dataSize;
//
//    strcpy(notifyData->module, this->Module.c_str());
//    strcpy(notifyData->notify, this->name.c_str());
//
//    char strTemp[CL_MAX_NAME_LENGTH];
//    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", this->name.c_str());
//    strcpy(data, strTemp);
//
//    map<std::string, std::string>::iterator mapIndex;
//    for (mapIndex = mLeafList.begin(); mapIndex != mLeafList.end(); ++mapIndex)
//    {
//      std::string leafName = (*mapIndex).first;
//      std::string leafVal = mLeafList[leafName];
//
//      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>",
//               leafName.c_str());
//      strcat(data, strTemp);
//      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "%s", leafVal.c_str());
//      strcat(data, strTemp);
//      snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>",
//               leafName.c_str());
//      strcat(data, strTemp);
//    }
//
//    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", this->name.c_str());
//    strcat(data, strTemp);
//    dataSize = strlen(data) + 1;
//
//    /*
//     * Send notification message to the NETCONF server
//     */
//    ClIocAddressT allNodeReps;
//    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
//    allNodeReps.iocPhyAddress.portId = SAFplusI::MGT_IOC_PORT;
//    MgtRoot::sendMsg(allNodeReps,notifyData,sizeof(ClMgtMessageNotifyTypeT) + dataSize,MgtMsgType::CL_MGT_MSG_NOTIF);

    free(buffer);
#endif
  }
}
