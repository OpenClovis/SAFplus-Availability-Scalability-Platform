// /*
//  * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
//  *
//  * This file is available  under  a  commercial  license  from  the
//  * copyright  holder or the GNU General Public License Version 2.0.
//  *
//  * The source code for  this program is not published  or otherwise
//  * divested of  its trade secrets, irrespective  of  what  has been
//  * deposited with the U.S. Copyright office.
//  *
//  * This program is distributed in the  hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//  * General Public License for more details.
//  *
//  * For more  information, see  the file  COPYING provided with this
//  * material.
//  */

#ifndef ALARM_MESSAGE_PROTOCOL_HXX_HEADER_INCLUDED
#define ALARM_MESSAGE_PROTOCOL_HXX_HEADER_INCLUDED
#include <AlarmMessageType.hxx>
#include <AlarmData.hxx>
#include <AlarmUtils.hxx>
using namespace SAFplusAlarm;
using namespace SAFplus;
namespace SAFplus
{

// alarm message protocol
class AlarmMessageProtocol
{
  public:
    // alarm message type
    AlarmMessageType messageType;
    // alarm data
    AlarmData alarmData;
    // alarm proifle
    MAPALARMPROFILEINFO alarmProfileData;
    //std::vector<AlarmProfileInfo> vectProfiles;
};
}


#endif /* ALARM_MESSAGE_PROTOCOL_HXX_HEADER_INCLUDED */
