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

#include "AlarmInfo.hxx"

using namespace SAFplusAlarm;

namespace SAFplus
{
AlarmInfo::AlarmInfo()
{
  memset(resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  category = AlarmCategory::INVALID;
  probCause = AlarmProbableCause::INVALID;
  severity = AlarmSeverity::INVALID;
  memset(syncData, 0, 1);
  state = AlarmState::INVALID;
  currentState = AlarmState::INVALID;
  afterSoakingBitmap = 0;
  timerType = TimerType::INVALID;
}

AlarmInfo& AlarmInfo::operator=(const AlarmInfo& other)
{
  memset(resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  strcpy(resourceId, other.resourceId);
  category = other.category;
  probCause = other.probCause;
  statusChangeTime = other.statusChangeTime;
  severity = other.severity;
  vectAltResource = other.vectAltResource;
  memset(syncData, 0, 1);
  state = other.state;
  currentState = other.currentState;
  timerType = other.timerType;
  startTimer = other.startTimer;
  afterSoakingBitmap = other.afterSoakingBitmap;
}
bool AlarmInfo::operator==(const AlarmInfo& other) const
{
  return ((strcmp(resourceId, other.resourceId) == 0) && (category == other.category) && (probCause == other.probCause) && (statusChangeTime == other.statusChangeTime) && (severity == other.severity) && (vectAltResource == other.vectAltResource) && (strcmp(syncData, other.syncData) == 0)
      && (state == other.state) && (currentState == other.currentState) && (startTimer == other.startTimer) &&(timerType == other.timerType)&&(afterSoakingBitmap == other.afterSoakingBitmap));
}

std::string AlarmInfo::toString() const
{
  std::ostringstream oss;
  oss << "resourceId:" << resourceId << " category:" << category << " probCause:" << probCause << " statusChangeTime:" << statusChangeTime << " severity:" << severity;
  if (vectAltResource.size() > 0)
  {
    oss << " altResource:[";
    int pos = 0;
    for (const std::string& altName : vectAltResource)
    {
      if (pos > 0) oss << ",";
      oss << altName;
      pos++;
    }
    oss << "]";
  }
  oss << " syncData:[" << syncData <<"] state:[" << state << "]"<< " currentState:["<<currentState<<"]"<<"] timerType:["<<timerType<< "] startTimer:["<<startTimer<< "] afterSoakingBitmap:"<<afterSoakingBitmap<<"]";
  return oss.str();
}
}
