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

#include "AlarmTimerInfo.hxx"

using namespace SAFplusAlarm;

namespace SAFplus
{
AlarmTimerInfo::AlarmTimerInfo()
{
  pTimer = nullptr;
  pAlarmData = nullptr;
}
AlarmTimerInfo::AlarmTimerInfo(const AlarmTimerInfo& other)
{
  if(nullptr != pTimer)
  {
    delete pTimer;
  }
  pTimer = other.pTimer;
  if(nullptr != pAlarmData)
  {
    delete pAlarmData;
  }
  pAlarmData = other.pAlarmData;
}
AlarmTimerInfo& AlarmTimerInfo::operator=(const AlarmTimerInfo& other)
{
  if(nullptr != pTimer)
  {
    delete pTimer;
  }
  if(nullptr != pAlarmData)
  {
    delete pAlarmData;
  }
  pTimer = other.pTimer;
  pAlarmData = other.pAlarmData;
}
bool AlarmTimerInfo::operator==(const AlarmTimerInfo& other) const
{
  return ((nullptr != pTimer)&&(nullptr != pAlarmData)&&(pTimer == other.pTimer)&&(pAlarmData != other.pAlarmData));
}

std::string AlarmTimerInfo::toString() const
{
  std::ostringstream oss;
  if(nullptr != pTimer)
  {
    oss <"Timer: Valid";
  }else{
    oss <<"Timer: Invalid";
  }
  if(nullptr != pAlarmData)
  {
    oss<<pAlarmData->toString();
  }else
  {
    oss<<"Data: Invalid";
  }
  return oss.str();
}
}
