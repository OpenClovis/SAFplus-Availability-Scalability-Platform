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
  memset(strText, 0, SAFplusI::MAX_TEXT_SIZE);
  memset(strOperator, 0, SAFplusI::MAX_TEXT_SIZE);
  memset(strOperatorText, 0, SAFplusI::MAX_TEXT_SIZE);
  state = AlarmState::INVALID;
  currentState = AlarmState::INVALID;
  afterSoakingBitmap = 0;
  sharedAssertTimer = nullptr;
  sharedClearTimer = nullptr;
  sharedAlarmData = nullptr;
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
  memset(strText, 0, SAFplusI::MAX_TEXT_SIZE);
  memset(strOperator, 0, SAFplusI::MAX_TEXT_SIZE);
  memset(strOperatorText, 0, SAFplusI::MAX_TEXT_SIZE);
  strcpy(strText, other.strText);
  strcpy(strOperator, other.strOperator);
  strcpy(strOperatorText, other.strOperatorText);
  operatorActionTime = other.operatorActionTime;
  state = other.state;
  currentState = other.currentState;
  afterSoakingBitmap = other.afterSoakingBitmap;
  sharedAssertTimer = other.sharedAssertTimer;
  sharedClearTimer = other.sharedClearTimer;
  sharedAlarmData = other.sharedAlarmData;
}
bool AlarmInfo::operator==(const AlarmInfo& other) const
{
  return ((strcmp(resourceId, other.resourceId) == 0) && (category == other.category) && (probCause == other.probCause) && (statusChangeTime == other.statusChangeTime) && (severity == other.severity) && (vectAltResource == other.vectAltResource) && (strcmp(strText, other.strText) == 0)
      && (strcmp(strOperator, other.strOperator) == 0) && (strcmp(strOperatorText, other.strOperatorText) == 0) && (operatorActionTime == other.operatorActionTime) && (state == other.state) &&(currentState == other.currentState) && (afterSoakingBitmap == other.afterSoakingBitmap));
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
  oss << " Text:[" << strText << "] lastOperator:[" << strOperator << "] operatorText:[" << strOperatorText << "] operatorActionTime:[" << operatorActionTime << "] state:[" << state << "]"<< " currentState:["<<currentState<<"]" << " afterSoakingBitmap:[" << afterSoakingBitmap << "]";
  return oss.str();
}
}
