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
#include "clMgtIoc.hxx"
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

#define clLog(...)

#define CL_IOC_MGT_NETCONF_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 1)
#define CL_IOC_MGT_SNMP_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 2)

ClMgtNotify::ClMgtNotify(const char* name)
{
    Name.assign(name);
    Module.assign("");
}

ClMgtNotify::~ClMgtNotify()
{
}

void ClMgtNotify::addLeaf(std::string leaf, std::string defaultValue)
{
    mLeafList.insert(pair<string, string>(leaf, defaultValue));
}
void ClMgtNotify::setLeaf(std::string leaf, std::string value)
{
    mLeafList[leaf] = value;
}

void ClMgtNotify::getLeaf(std::string leaf, std::string *value)
{
    *value = mLeafList[leaf];
}

void ClMgtNotify::sendNotification()
{
    if (!strcmp(Module.c_str(), ""))
    {
        clLogError("MGT", "RPC", "Cannot send Notification [%s]", Name.c_str());
        return;
    }
#ifdef MGT_ACCESS
    ClMgtIoc* mgtIocInstance = ClMgtIoc::getInstance();

    char *buffer = (char *) malloc(MGT_MAX_DATA_LEN);
    if (!buffer)
    {
        return;
    }

    ClMgtMessageNotifyTypeT *notifyData = (ClMgtMessageNotifyTypeT *)buffer;
    ClCharT *data = notifyData->data;
    ClUint32T dataSize;

    strcpy(notifyData->module, this->Module.c_str());
    strcpy(notifyData->notify, this->Name.c_str());

    char strTemp[CL_MAX_NAME_LENGTH];
    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>", this->Name.c_str());
    strcpy(data, strTemp);

    map<std::string, std::string>::iterator mapIndex;
    for (mapIndex = mLeafList.begin(); mapIndex != mLeafList.end(); ++mapIndex)
    {
        std::string leafName = (*mapIndex).first;
        std::string leafVal = mLeafList[leafName];

        snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "<%s>",
                leafName.c_str());
        strcat(data, strTemp);
        snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "%s", leafVal.c_str());
        strcat(data, strTemp);
        snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>",
                leafName.c_str());
        strcat(data, strTemp);

    }

    snprintf((char *) strTemp, CL_MAX_NAME_LENGTH, "</%s>", this->Name.c_str());
    strcat(data, strTemp);
    dataSize = strlen(data) + 1;

    /*
     * TODO: switch to Async
     *
     * Send notification message to the NETCONF server
     */
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_NETCONF_PORT;
    mgtIocInstance->sendIocMsgAsync(&allNodeReps, CL_MGT_MSG_NOTIF, notifyData,
            sizeof(ClMgtMessageNotifyTypeT) + dataSize, NULL, NULL);

    /*
     * TODO: switch to Async
     *
     * Send notification message to the SNMP master
     */
    allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_SNMP_PORT;

    mgtIocInstance->sendIocMsgAsync(&allNodeReps, CL_MGT_MSG_NOTIF, notifyData,
            sizeof(ClMgtMessageNotifyTypeT) + dataSize, NULL, NULL);

    free(buffer);
#endif
}
